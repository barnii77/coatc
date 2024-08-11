; ModuleID = 'test/llvmir_out/test1.bpl.ll'
source_filename = "test/code_samples/test1.bpl"

; Function Attrs: noreturn
define noundef i8 @main(i8 %b) local_unnamed_addr #0 {
declarations_block:
  br label %cond_block

cond_block:                                       ; preds = %cond_block, %declarations_block
  %calltmp11 = tail call i8 @print(i8 -66)
  br label %cond_block
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define i8 @add1(i8 %x) local_unnamed_addr #1 {
declarations_block:
  %addtmp = add i8 %x, 1
  ret i8 %addtmp
}

declare i8 @print(i8) local_unnamed_addr

attributes #0 = { noreturn }
attributes #1 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) }
