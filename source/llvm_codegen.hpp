#pragma once

#include "ast.hpp"
#include "lib.hpp"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <optional>

namespace codegen {
typedef struct Error {
    LocationInfo loc;
    std::string msg;
} Error;

typedef struct State {
    std::unordered_map<std::string, llvm::AllocaInst*> named_values{};
    llvm::BasicBlock *declarations_block = nullptr;
} State;

typedef struct Context {
    std::unique_ptr<llvm::LLVMContext> llvm_ctx;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    std::unique_ptr<llvm::Module> module;
    std::vector<Error> *errors = nullptr;
    State *state = nullptr;
} Context;

Context newContext(StringRef file, std::vector<Error> *errs, State *cg_state);

class CodeGenException : public std::exception {
public:
    std::string m_message;
    LocationInfo m_loc;

    CodeGenException(std::string message, LocationInfo loc);
    char const *what();
};

std::string dumpIR(Context const *ctx);
}  // namespace codegen
