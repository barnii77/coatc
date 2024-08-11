#include "LLVMCodeGen/optimization.hpp"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/OptimizationLevel.h"
#include "llvm/Transforms/Utils/FunctionImportUtils.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Support/raw_ostream.h"  // TODO remove once debugged
#include <iostream>

void codegen::optimizeModule(llvm::Module &module, OptLevel opt_level_) {
    auto opt_level = llvm::OptimizationLevel::O0;
    if (opt_level_ == OptLevel::O1)
        opt_level = llvm::OptimizationLevel::O1;
    else if (opt_level_ == OptLevel::O2)
        opt_level = llvm::OptimizationLevel::O2;
    else if (opt_level_ == OptLevel::Os)
        opt_level = llvm::OptimizationLevel::Os;
    else if (opt_level_ == OptLevel::Oz)
        opt_level = llvm::OptimizationLevel::Oz;
    else if (opt_level_ == OptLevel::O3)
        opt_level = llvm::OptimizationLevel::O3;

    llvm::LoopAnalysisManager loop_analysis_manager;
    llvm::FunctionAnalysisManager function_analysis_manager;
    llvm::CGSCCAnalysisManager cgscc_analysis_manager;
    llvm::ModuleAnalysisManager module_analysis_manager;
    llvm::PassBuilder pass_builder;

    pass_builder.registerModuleAnalyses(module_analysis_manager);
    pass_builder.registerCGSCCAnalyses(cgscc_analysis_manager);
    pass_builder.registerFunctionAnalyses(function_analysis_manager);
    pass_builder.registerLoopAnalyses(loop_analysis_manager);
    pass_builder.crossRegisterProxies(loop_analysis_manager, function_analysis_manager, cgscc_analysis_manager, module_analysis_manager);

    llvm::ModulePassManager module_pass_manager = pass_builder.buildPerModuleDefaultPipeline(opt_level);
    std::string oss;
    llvm::raw_string_ostream os(oss);
    module_pass_manager.printPipeline(os, [](llvm::StringRef name) -> llvm::StringRef {return name;});
    std::cout << "PIPELINE:" << std::endl << oss << std::endl;

    std::cout << "IR DUMP:" << std::endl;
    std::string tmp;
    llvm::raw_string_ostream os2(tmp);
    os2 << module;
    std::cout << tmp << std::endl;
    std::cout << "END IR DUMP" << std::endl;
    module_pass_manager.run(module, module_analysis_manager);
}
