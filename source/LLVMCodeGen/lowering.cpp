#include "LLVMCodeGen/lowering.hpp"
#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/LTO/LTO.h"
#include "llvm/LTO/legacy/LTOModule.h"
#include "llvm/LTO/legacy/LTOCodeGenerator.h"
#include "llvm/LTO/legacy/ThinLTOCodeGenerator.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Linker/Linker.h"
#include <stdexcept>
#include <iostream>
#include <cstdlib>

template<typename T>
[[noreturn]] T unreachable() {
    throw std::runtime_error("unreachable statement reached");
}

void codegen::moduleSetTargetMachine(llvm::Module *module, llvm::TargetMachine *target_machine) {
    module->setTargetTriple(target_machine->getTargetTriple().getTriple());
    module->setDataLayout(target_machine->createDataLayout());
}

llvm::TargetMachine *codegen::setupTargetMachine(
    llvm::Module *module,
    OptLevel opt_level_,
    bool disallow_fp_op_fusion,
    std::optional<std::string> target_triple_,
    std::string const &target_cpu,
    std::string const &target_features
) {
    auto opt_level = llvm::CodeGenOptLevel::None;
    if (opt_level_ == OptLevel::O1)
        opt_level = llvm::CodeGenOptLevel::Less;
    else if (opt_level_ == OptLevel::O2)
        opt_level = llvm::CodeGenOptLevel::Default;
    else if (opt_level_ == OptLevel::Os)
        opt_level = llvm::CodeGenOptLevel::Default;
    else if (opt_level_ == OptLevel::Oz)
        opt_level = llvm::CodeGenOptLevel::Default;
    else if (opt_level_ == OptLevel::O3)
        opt_level = llvm::CodeGenOptLevel::Aggressive;

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
    
    std::string default_target_triple = llvm::sys::getDefaultTargetTriple();
    std::string target_triple;
    if (target_triple_)
        target_triple = std::move(target_triple_.value());
    else
        target_triple = std::move(default_target_triple);

    std::string error;
    llvm::Target const *target = llvm::TargetRegistry::lookupTarget(target_triple, error);
    if (!target) {
        std::cerr << "Error: " << error << "\n";
        throw std::runtime_error("critical failure");
    }
    
    llvm::TargetOptions target_options;
    // some settings to increase performance
    if (opt_level_ == OptLevel::O3) {
        target_options.EnableFastISel = 0;
        // TODO add additional flags to control this
        // target_options.UnsafeFPMath = true;
        // target_options.NoInfsFPMath = true;
        // target_options.NoNaNsFPMath = true;
        if (!disallow_fp_op_fusion)
            target_options.AllowFPOpFusion = llvm::FPOpFusion::Fast;
    }

    llvm::TargetMachine *target_machine = target->createTargetMachine(
        target_triple,
        target_cpu,
        target_features,
        target_options,
        llvm::Reloc::PIC_,  // std::optional<llvm::Reloc::Model>()
        std::nullopt,  // std::optional<llvm::CodeModel::Model>()
        opt_level
    );
    
    if (!target_machine) {
        std::cerr << "Error: Unable to create target machine.\n";
        throw std::runtime_error("critical failure");
    }
    
    if (module) {
        codegen::moduleSetTargetMachine(module, target_machine);
    }

    return target_machine;
}

// TODO thin lto
// TODO regular lto
std::string codegen::lowerModulesToExeWithLTO(
    std::vector<std::unique_ptr<llvm::Module>> modules,
    llvm::TargetMachine *target_machine
) {
    // auto llvm_ctx = std::make_unique<llvm::LLVMContext>();
    // auto combined_module = std::make_unique<llvm::Module>(llvm::StringRef("<output>"), *llvm_ctx);

    // lto_config.Options = target_machine->Options;
    // lto_config.CGOptLevel = target_machine->getOptLevel();
    // lto_config.OptLevel = static_cast<unsigned>(target_machine->getOptLevel());
    // lto_config.RelocModel = target_machine->getRelocationModel();
    // lto_config.CodeModel = target_machine->getCodeModel();
    // lto_config.CPU = target_machine->getTargetCPU();
    // llvm::StringRef features = target_machine->getTargetFeatureString();
    // llvm::SmallVector<llvm::StringRef, 8> featureList;
    // features.split(featureList, ',');
    // std::vector<std::string> attrs;
    // for (const auto &feature : featureList) {
    //     attrs.push_back(feature.str());
    // }
    // lto_config.MAttrs = std::move(attrs);

    // llvm::LTOCodeGenerator lto_cg(*llvm_ctx);
    // lto_cg.setModule(combined_module);

    // for (auto &module : modules) {
    //     if (llvm::verifyModule(*module)) {
    //         std::cerr << "Error: module verification failed\n";
    //         throw std::runtime_error("irrecoverable exception");
    //     }
    //     // TODO instruction manual at llvm/include/llvm/LTO/LTO.h:250
    //     if (lto_backend.add(std::move(module)) != llvm::Error::success()) {
    //         throw std::runtime_error("Error adding module to LTO Backend");
    //     }
    // }

    // std::string mc;
    // llvm::raw_string_ostream mc_stream(mc);
    // llvm::buffer_ostream mcbs(mc_stream);

    // llvm::lto::ThinBackend lto_thin_backend;
    // if (lto_backend.run(lto_thin_backend, mcbs) != llvm::Error::success()) {
    //     throw std::runtime_error("LTO backend failed (might be a compiler bug...)");
    // }

    // mc_stream.flush();
    // return mc;
    throw std::runtime_error("not implemented");
}

std::string codegen::lowerModuleToFormatNoLinking(
    llvm::Module *module,
    llvm::TargetMachine *target_machine,
    codegen::LoweringOutKind out_kind
) {
    if (out_kind == codegen::LoweringOutKind::combined_obj)
        throw std::runtime_error("lowerModuleToFormatNoLinking called with invalid argument out_kind = combined_obj; should be handled in caller");
    if (out_kind == codegen::LoweringOutKind::bitcode) {
        llvm::SmallVector<char> buf;
        llvm::BitcodeWriter bcw(buf);
        bcw.writeModule(*module);
        std::string bitcode(buf.data(), buf.size());
        return bitcode;
    }

    auto filetype = out_kind == codegen::LoweringOutKind::obj
        ? llvm::CodeGenFileType::ObjectFile
        : (
            out_kind == codegen::LoweringOutKind::asm_
            ? llvm::CodeGenFileType::AssemblyFile 
            : unreachable<llvm::CodeGenFileType>()
        );

    std::string asm_out;
    llvm::raw_string_ostream asm_string_ostream(asm_out);
    llvm::buffer_ostream asm_buffer_stream(asm_string_ostream);

    llvm::legacy::PassManager pm;
    if (target_machine->addPassesToEmitFile(pm, asm_buffer_stream, nullptr, filetype)) {
        std::cerr << "TargetMachine can't emit a file of this type";
        throw std::runtime_error("irrecoverable exception");
    }

    if (llvm::verifyModule(*module, &llvm::errs())) {
        std::cerr << "Error: Module verification failed.\n";
        throw std::runtime_error("irrecoverable exception");
    }

    pm.run(*module);
    return asm_out;
}

std::vector<std::string> codegen::lowerModulesToFormatNoLTO(
    std::vector<std::unique_ptr<llvm::Module>> modules,
    llvm::TargetMachine *target_machine,
    codegen::LoweringOutKind out_kind,
    std::optional<std::function<void(llvm::Module*)>> combined_module_callback
) {
    if (out_kind == codegen::LoweringOutKind::bitcode) {
        std::vector<std::string> out;
        for (auto &module : modules)
            out.push_back(codegen::lowerModuleToFormatNoLinking(module.get(), target_machine, codegen::LoweringOutKind::bitcode));
        return out;
    }

    if (out_kind != codegen::LoweringOutKind::combined_obj) {
        std::vector<std::string> out;
        for (auto &module : modules)
            out.push_back(codegen::lowerModuleToFormatNoLinking(module.get(), target_machine, out_kind));
        return out;
    }

    auto llvm_ctx = std::make_unique<llvm::LLVMContext>();
    auto combined_module = std::make_unique<llvm::Module>(llvm::StringRef("<output>"), *llvm_ctx);

    llvm::Linker linker(*combined_module);
    for (auto &module : modules) {
        if (linker.linkInModule(std::move(module))) {
            std::cerr << "cross-module linking failed";
            throw std::runtime_error("irrecoverable exception: cross-module linking failed");
        }
    }

    if (combined_module_callback)
        combined_module_callback.value()(combined_module.get());

    std::string combined_lowered = codegen::lowerModuleToFormatNoLinking(combined_module.get(), target_machine, codegen::LoweringOutKind::obj);
    return std::vector<std::string>{combined_lowered};
}
