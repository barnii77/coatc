#include "llvm/IR/Module.h"
#include "llvm/Target/TargetMachine.h"
#include "lib.hpp"
#include <memory>
#include <vector>
#include <cstdint>

namespace codegen {
void optimizeModule(
    llvm::Module *module,
    llvm::TargetMachine *target_machine = nullptr,
    OptLevel opt_level_ = OptLevel::O0,
    uint32_t n_max_pipeline_runs = 1
);

void batchOptimizeModules(
    std::vector<llvm::Module*> module,
    llvm::TargetMachine *target_machine = nullptr,
    OptLevel opt_level_ = OptLevel::O0,
    uint32_t n_max_pipeline_runs = 1
);
}  // namespace codegen
