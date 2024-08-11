#include "lib.hpp"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"
#include <string>

namespace codegen {
std::string lowerToAssembly(
    llvm::Module &module,
    OptLevel opt_level = OptLevel::O0,
    std::string const &target_triple = llvm::sys::getDefaultTargetTriple(),
    std::string const &target_cpu = "generic",
    std::string const &target_features = ""
);
}  // namespace codegen
