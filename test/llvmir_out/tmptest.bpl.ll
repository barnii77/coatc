; ModuleID = 'test/code_samples/tmptest.bpl'
source_filename = "test/code_samples/tmptest.bpl"

define i8 @main(i8 %b) {
declarations_block:
  %b1 = alloca i8, align 1
  %a = alloca i8, align 1
  %if_result = alloca i8, align 1
  br label %entry

entry:                                            ; preds = %declarations_block
  br label %block_lifetimes_start

block_lifetimes_start:                            ; preds = %entry
  call void @llvm.lifetime.start(i64 8, ptr %a)
  br label %block_entry

block_entry:                                      ; preds = %block_lifetimes_start
  store i8 3, ptr %a, align 1
  %a.loadtmp = load i8, ptr %a, align 1
  %multmp = mul i8 %a.loadtmp, 63
  %calltmp = call i8 @add1(i8 %multmp)
  store i8 %calltmp, ptr %b1, align 1
  br i1 true, label %cond_true, label %cond_false

cond_true:                                        ; preds = %block_entry
  br label %block_lifetimes_start2

block_lifetimes_start2:                           ; preds = %cond_true
  br label %block_entry3

block_entry3:                                     ; preds = %block_lifetimes_start2
  ret i8 1
  br label %block_lifetimes_end

block_lifetimes_end:                              ; preds = %block_entry3
  store i8 0, ptr %if_result, align 1
  br label %post_if

cond_false:                                       ; preds = %block_entry
  br label %block_lifetimes_start4

block_lifetimes_start4:                           ; preds = %cond_false
  br label %block_entry5

block_entry5:                                     ; preds = %block_lifetimes_start4
  br label %block_lifetimes_end6

block_lifetimes_end6:                             ; preds = %block_entry5
  store i8 0, ptr %if_result, align 1
  br label %post_if

post_if:                                          ; preds = %block_lifetimes_end6, %block_lifetimes_end
  %if_result.loadtmp = load i8, ptr %if_result, align 1
  br label %cond_block

cond_block:                                       ; preds = %block_lifetimes_end11, %post_if
  %b.loadtmp = load i8, ptr %b1, align 1
  %0 = icmp ne i8 %b.loadtmp, 0
  br i1 %0, label %loop_body, label %post_while

loop_body:                                        ; preds = %cond_block
  br label %block_lifetimes_start7

block_lifetimes_start7:                           ; preds = %loop_body
  br label %block_entry8

block_entry8:                                     ; preds = %block_lifetimes_start7
  %b.loadtmp9 = load i8, ptr %b1, align 1
  %calltmp10 = call i8 @print(i8 %b.loadtmp9)
  br label %block_lifetimes_end11

block_lifetimes_end11:                            ; preds = %block_entry8
  br label %cond_block

post_while:                                       ; preds = %cond_block
  %a.loadtmp12 = load i8, ptr %a, align 1
  %calltmp13 = call i8 @print(i8 %a.loadtmp12)
  ret i8 105
  br label %block_lifetimes_end14

block_lifetimes_end14:                            ; preds = %post_while
  call void @llvm.lifetime.end(i64 8, ptr %a)
  ret i8 0
}

define i8 @add1(i8 %x) {
declarations_block:
  %x1 = alloca i8, align 1
  br label %entry

entry:                                            ; preds = %declarations_block
  br label %block_lifetimes_start

block_lifetimes_start:                            ; preds = %entry
  br label %block_entry

block_entry:                                      ; preds = %block_lifetimes_start
  %x.loadtmp = load i8, ptr %x1, align 1
  %addtmp = add i8 %x.loadtmp, 1
  ret i8 %addtmp
  br label %block_lifetimes_end

block_lifetimes_end:                              ; preds = %block_entry
  ret i8 0
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start(i64 immarg, ptr nocapture) #0

declare i8 @print(i8)

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end(i64 immarg, ptr nocapture) #0

attributes #0 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
