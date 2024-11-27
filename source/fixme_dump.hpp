// TODO extend this function to also support regular LTO
// TODO check out:
// clang: BackendUtil.cpp:1036
// llvm docs: https://llvm.org/doxygen/classllvm_1_1PassBuilder.html#a71c04f1bf2b6ebcb3431b9f159b3921e
// clang: LTO.h:111
std::string codegen::lowerModulesToExeWithLTO(
    std::vector<std::unique_ptr<llvm::Module>> modules,
    llvm::TargetMachine *target_machine
) {
    llvm::lto::Config lto_config;

    // lto configuration
    lto_config.Options = target_machine->Options;
    lto_config.CGOptLevel = target_machine->getOptLevel();
    lto_config.OptLevel = static_cast<unsigned>(target_machine->getOptLevel());
    lto_config.RelocModel = target_machine->getRelocationModel();
    lto_config.CodeModel = target_machine->getCodeModel();
    lto_config.CPU = target_machine->getTargetCPU();
    llvm::StringRef features = target_machine->getTargetFeatureString();
    llvm::SmallVector<llvm::StringRef, 8> featureList;
    features.split(featureList, ',');
    std::vector<std::string> attrs;
    for (const auto &feature : featureList) {
        attrs.push_back(feature.str());
    }
    lto_config.MAttrs = std::move(attrs);

    llvm::lto::LTO lto_backend(std::move(lto_config));

    for (auto &module : modules) {
        if (llvm::verifyModule(*module)) {
            std::cerr << "Error: module verification failed\n";
            throw std::runtime_error("irrecoverable exception");
        }
        // TODO instruction manual at llvm/include/llvm/LTO/LTO.h:250
        if (lto_backend.add(std::move(module)) != llvm::Error::success()) {
            throw std::runtime_error("Error adding module to LTO Backend");
        }
    }

    std::string mc;
    llvm::raw_string_ostream mc_stream(mc);
    llvm::buffer_ostream mcbs(mc_stream);

    llvm::lto::ThinBackend lto_thin_backend;
    if (lto_backend.run(lto_thin_backend, mcbs) != llvm::Error::success()) {
        throw std::runtime_error("LTO backend failed (might be a compiler bug...)");
    }

    mc_stream.flush();
    return mc;
}
