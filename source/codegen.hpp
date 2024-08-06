#include "ast.hpp"
// TODO include llvm

namespace ast {
struct CodeGenContext {
    std::unique_ptr<LLVMContext> llvm_context;
    std::unique_ptr<IRBuilder<>> llvm_builder;
    std::unique_ptr<Module> llvm_module;
    std::unordered_map<std::string, Value*> named_values;
    std::vector<CodeGenError> *errors;
};
}  // namespace ast
