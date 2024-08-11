#include "llvm/IR/Module.h"
#include "lib.hpp"

namespace codegen {
void optimizeModule(llvm::Module &module, OptLevel opt_level_ = OptLevel::O0);
}  // namespace codegen
