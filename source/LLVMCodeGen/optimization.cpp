#include "LLVMCodeGen/optimization.hpp"
#include "LLVMCodeGen/cleanup_pass.hpp"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/OptimizationLevel.h"
#include "llvm/Transforms/Scalar/DCE.h"
#include "llvm/Transforms/Utils/FunctionImportUtils.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/IR/Verifier.h"
#include <iostream>
#include <optional>
#include <string>

void printPipeline(llvm::ModulePassManager &mpm) {
    std::string str;
    llvm::raw_string_ostream sos(str);
    mpm.printPipeline(sos, [](llvm::StringRef name) -> llvm::StringRef {return name;});
    std::cout << "PIPELINE:" << std::endl << str << std::endl;
}

void writeModuleToString(llvm::Module *module, std::string &out) {
    out.clear();
    llvm::raw_string_ostream os(out);
    os << *module;
    os.flush();
}

llvm::PipelineTuningOptions getPipelineTuningOptions(OptLevel const opt_level) {
    llvm::PipelineTuningOptions tuning_options;
    if (opt_level >= OptLevel::O2) {
        tuning_options.LoopUnrolling = true;
        tuning_options.LoopInterleaving = true;
        tuning_options.LoopVectorization = true;
        tuning_options.SLPVectorization = true;
    }
    return tuning_options;
}

void codegen::optimizeModule(
    llvm::Module *module,
    llvm::TargetMachine *target_machine,
    OptLevel const opt_level_,
    uint32_t n_max_pipeline_runs
) {
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

    std::string pre_mod_ir;
    std::string post_mod_ir;
    // with O3 opt level, the opt passes are run until there are no changes in the IR anymore
    bool is_first_run = true;
    do {
        llvm::LoopAnalysisManager lam;
        llvm::FunctionAnalysisManager fam;
        llvm::CGSCCAnalysisManager cgam;
        llvm::ModuleAnalysisManager mam;
        llvm::PassInstrumentationCallbacks pass_instrumentation_callbacks;

        auto tuning_options = getPipelineTuningOptions(opt_level_);
        llvm::PassBuilder pb(target_machine, tuning_options);

        pb.registerModuleAnalyses(mam);
        pb.registerCGSCCAnalyses(cgam);
        pb.registerFunctionAnalyses(fam);
        pb.registerLoopAnalyses(lam);
        pb.crossRegisterProxies(lam, fam, cgam, mam);

        llvm::ModulePassManager mpm;
        if (is_first_run)
            mpm.addPass(codegen::CleanupPass());
        mpm.addPass(llvm::VerifierPass());
        mpm.addPass(pb.buildPerModuleDefaultPipeline(opt_level));

        if (opt_level_ == OptLevel::O3 && n_max_pipeline_runs > 1)
            pre_mod_ir = post_mod_ir;

        mpm.run(*module, mam);

        if (opt_level_ == OptLevel::O3 && n_max_pipeline_runs > 1)
            writeModuleToString(module, post_mod_ir);

        n_max_pipeline_runs--;
        is_first_run = false;
    } while (opt_level_ == OptLevel::O3 && n_max_pipeline_runs && pre_mod_ir != post_mod_ir);
}

// TODO try to find a way to reuse all those opt-related llvm objects
void codegen::batchOptimizeModules(
    std::vector<llvm::Module*> modules,
    llvm::TargetMachine *target_machine,
    OptLevel const opt_level_,
    uint32_t n_max_pipeline_runs
) {
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

    for (auto module : modules) {
        std::string pre_mod_ir;
        std::string post_mod_ir;
        // with O3 opt level, the opt passes are run until there are no changes in the IR anymore
        bool is_first_run = true;
        do {
            llvm::LoopAnalysisManager lam;
            llvm::FunctionAnalysisManager fam;
            llvm::CGSCCAnalysisManager cgam;
            llvm::ModuleAnalysisManager mam;
            llvm::PassInstrumentationCallbacks pass_instrumentation_callbacks;

            auto tuning_options = getPipelineTuningOptions(opt_level_);
            llvm::PassBuilder pb(target_machine, tuning_options);

            pb.registerModuleAnalyses(mam);
            pb.registerCGSCCAnalyses(cgam);
            pb.registerFunctionAnalyses(fam);
            pb.registerLoopAnalyses(lam);
            pb.crossRegisterProxies(lam, fam, cgam, mam);

            llvm::ModulePassManager mpm;
            if (is_first_run)
                mpm.addPass(codegen::CleanupPass());
            mpm.addPass(llvm::VerifierPass());
            mpm.addPass(pb.buildPerModuleDefaultPipeline(opt_level));

            if (opt_level_ == OptLevel::O3 && n_max_pipeline_runs > 1)
                pre_mod_ir = post_mod_ir;

            mpm.run(*module, mam);

            if (opt_level_ == OptLevel::O3 && n_max_pipeline_runs > 1)
                writeModuleToString(module, post_mod_ir);

            n_max_pipeline_runs--;
            is_first_run = false;
        } while (opt_level_ == OptLevel::O3 && n_max_pipeline_runs && pre_mod_ir != post_mod_ir);
    }
}
