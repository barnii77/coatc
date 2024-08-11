; ModuleID = 'test/code_samples/test1.bpl'
source_filename = "test/code_samples/test1.bpl"

define noundef i8 @main(i8 %b) local_unnamed_addr {
declarations_block:
  ret i8 -66
  %calltmp14 = tail call i8 @print(i8 3)
  ret i8 105
  ret i8 0
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define noundef i8 @add1(i8 %x) local_unnamed_addr #0 {
declarations_block:
  %addtmp = add i8 %x, 1
  ret i8 %addtmp
  ret i8 0
}

declare i8 @print(i8) local_unnamed_addr

attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) }
