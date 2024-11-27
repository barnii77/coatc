#include "lib.hpp"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"
#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <functional>

namespace codegen {
enum class LoweringOutKind {
    asm_,
    bitcode,
    obj,
    combined_obj
};

void moduleSetTargetMachine(
    llvm::Module *module,
    llvm::TargetMachine *target_machine
);

llvm::TargetMachine *setupTargetMachine(
    llvm::Module *module = nullptr,
    OptLevel opt_level = OptLevel::O0,
    bool disallow_fp_op_fusion = false,
    std::optional<std::string> target_triple_ = std::nullopt,
    std::string const &target_cpu = "generic",
    std::string const &target_features = ""
);

std::string lowerModulesToExeWithLTO(
    std::vector<std::unique_ptr<llvm::Module>> modules,
    llvm::TargetMachine *target_machine
);

std::string lowerModuleToFormatNoLinking(
    llvm::Module *module,
    llvm::TargetMachine *target_machine,
    codegen::LoweringOutKind out_kind
);

std::vector<std::string> lowerModulesToFormatNoLTO(
    std::vector<std::unique_ptr<llvm::Module>> modules,
    llvm::TargetMachine *target_machine,
    LoweringOutKind out_kind = LoweringOutKind::obj,
    // may be used to run opt pipeline across combined module before lowering (essentially hacky FullLTO)
    std::optional<std::function<void(llvm::Module*)>> combined_module_callback = std::nullopt
);
}  // namespace codegen
