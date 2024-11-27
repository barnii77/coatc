#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Module.h"

namespace codegen {
/// post-codegen cleanup pass that removes all instructions after the first terminator in a basic block
struct CleanupPass : public llvm::PassInfoMixin<CleanupPass> {
    llvm::PreservedAnalyses run(llvm::Module &module, llvm::ModuleAnalysisManager&);
};

llvm::PassPluginLibraryInfo getCleanupPassPluginInfo();
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo();
}  // namespace codegen
