#include "LLVMCodeGen/lowering.hpp"
#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/Support/FormattedStream.h"

std::string codegen::lowerToAssembly(
    llvm::Module &module,
    OptLevel opt_level_,
    std::string const &target_triple,
    std::string const &target_cpu,
    std::string const &target_features
) {
    auto opt_level = llvm::CodeGenOpt::Level::None;
    if (opt_level_ == OptLevel::O1)
        opt_level = llvm::CodeGenOpt::Level::Less;
    else if (opt_level_ == OptLevel::O2)
        opt_level = llvm::CodeGenOpt::Level::Default;
    else if (opt_level_ == OptLevel::Os)
        opt_level = llvm::CodeGenOpt::Level::Default;
    else if (opt_level_ == OptLevel::Oz)
        opt_level = llvm::CodeGenOpt::Level::Default;
    else if (opt_level_ == OptLevel::O3)
        opt_level = llvm::CodeGenOpt::Level::Aggressive;

    std::string error;
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargetMachines();
    llvm::InitializeAllTargets();
    llvm::InitializeAllAsmParsers();
    
    llvm::Target const *target = llvm::TargetRegistry::lookupTarget(target_triple, error);
    if (!target) {
        llvm::errs() << "Error: " << error << "\n";
        throw std::runtime_error("critical failure");
    }
    
    llvm::TargetOptions target_options;
    std::unique_ptr<llvm::TargetMachine> target_machine(
        target->createTargetMachine(
            target_triple, target_cpu, target_features, target_options, llvm::Optional<llvm::Reloc::Model>(), llvm::Optional<llvm::CodeModel::Model>(), opt_level
        )
    );
    
    if (!target_machine) {
        llvm::errs() << "Error: Unable to create target machine.\n";
        throw std::runtime_error("critical failure");
    }
    
    if (llvm::verifyModule(module, &llvm::errs())) {
        llvm::errs() << "Error: Module verification failed.\n";
        throw std::runtime_error("critical failure");
    }

    llvm::legacy::PassManager pass_manager;
    llvm::legacy::FunctionPassManager function_pass_manager(&module);

    llvm::TargetLoweringObjectFile &tlof = target_machine->getTargetLowering()->getObjFileLowering();
    // pass_manager.add(llvm::createPrintModulePass(llvm::outs()));  // Debug out
    pass_manager.add(llvm::createTargetTransformInfoWrapperPass(target_machine->getTargetIRAnalysis()));
    
    std::string assembly_output;
    llvm::raw_string_ostream assembly_stream(assembly_output);
    std::unique_ptr<llvm::AsmPrinter> asm_printer(target_machine->createAsmPrinter(assembly_stream, module));
    
    asm_printer->doInitialization();
    asm_printer->printModule(module);
    asm_printer->doFinalization();
    
    return assembly_output;
}
