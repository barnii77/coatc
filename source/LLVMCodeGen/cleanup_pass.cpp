#include "LLVMCodeGen/cleanup_pass.hpp"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

llvm::PreservedAnalyses codegen::CleanupPass::run(llvm::Module &module, llvm::ModuleAnalysisManager&) {
    for (auto &fn : module) {
        for (auto &bb : fn) {
            bool found_terminator = false;
            for (auto it = bb.begin(), end = bb.end(); it != end;) {
                if (found_terminator) {
                    it = it->eraseFromParent();
                } else if (it->isTerminator()) {
                    found_terminator = true;
                    ++it;
                } else {
                    ++it;
                }
            }
        }
    }
    return llvm::PreservedAnalyses::none();
}

/// Pass registration for use as a plugin
llvm::PassPluginLibraryInfo codegen::getCleanupPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "CleanupPass", LLVM_VERSION_STRING,
        [](llvm::PassBuilder &pb) {
            pb.registerPipelineParsingCallback(
                [](llvm::StringRef name, llvm::ModulePassManager &mpm,
                   llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                    if (name == "cleanup") {
                        mpm.addPass(CleanupPass());
                        return true;
                    }
                    return false;
                });
        }
    };
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo codegen::llvmGetPassPluginInfo() {
    return getCleanupPassPluginInfo();
}
