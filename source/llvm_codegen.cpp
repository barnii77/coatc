/* This is the llvm codegen backend of the bpl compiler. Some key info about it:
 * All return types from functions are
 */

#include "llvm_codegen.hpp"

void assertNonNull(void *ptr) {
    if (!ptr)
        throw std::runtime_error("non-null assertion for pointer failed");
}

std::string codegen::dumpIR(codegen::Context const *ctx) {
    std::string tmp;
    llvm::raw_string_ostream os(tmp);
    os << *ctx->module;
    return tmp;
}

std::string llvmTypeAsString(llvm::Type const *ty) {
    std::string tmp;
    llvm::raw_string_ostream os(tmp);
    ty->print(os);
    return tmp;
}

llvm::AllocaInst *allocaInDeclBlock(codegen::Context *ctx, llvm::Type *ty, char const *name) {
    assertNonNull(ctx->state->declarations_block);
    auto saved_ip = ctx->builder->saveIP();
    ctx->builder->SetInsertPoint(ctx->state->declarations_block);
    llvm::AllocaInst *alloca = ctx->builder->CreateAlloca(ty, nullptr, name);
    ctx->builder->restoreIP(saved_ip);
    return alloca;
}

codegen::CodeGenException::CodeGenException(std::string message, LocationInfo loc)
    : m_message(std::move(message)), m_loc(loc)
{}

char const *codegen::CodeGenException::what() {
    return m_message.c_str();
}

bool variableDefined(codegen::Context *ctx, std::string const &variable) {
    return ctx->state->named_values.find(variable) != ctx->state->named_values.end();
}

void createPrototype(codegen::Context *ctx, ast::FunctionProto const *proto) {
    std::vector<llvm::Type*> arg_types(proto->args.size(), ctx->builder->getInt8Ty());
    llvm::FunctionType *fty = llvm::FunctionType::get(ctx->builder->getInt8Ty(), arg_types, false);
    llvm::Function *fn = llvm::Function::Create(fty, llvm::Function::ExternalLinkage, proto->name, ctx->module.get());
    uint32_t i = 0;
    for (auto &arg : fn->args()) {
        arg.setName(proto->args[i++]);
    }
}

codegen::Context codegen::newContext(StringRef file, std::vector<codegen::Error> *errs, std::vector<codegen::Warning> *warns, codegen::State *cg_state) {
    auto result = codegen::Context {
        .state = cg_state,
        .errors = errs,
        .warnings = warns,
    };
    result.llvm_ctx = std::make_unique<llvm::LLVMContext>();
    result.builder = std::make_unique<llvm::IRBuilder<>>(*result.llvm_ctx);
    result.module = std::make_unique<llvm::Module>(llvm::StringRef(file.start, file.length), *result.llvm_ctx);
    return result;
}

void *ast::Expr::codegen(void *ctx_) const {
    throw std::runtime_error("called codegen on abstract class ast::Expr");
}

void *ast::Statement::codegen(void *ctx_) const {
    throw std::runtime_error("called codegen on abstract class ast::Statement");
}

void *ast::BinaryOp::codegen(void *ctx_) const {
    codegen::Context *ctx = static_cast<codegen::Context*>(ctx_);
    llvm::Value *lhs = static_cast<llvm::Value*>(m_lhs->codegen(ctx));
    llvm::Value *rhs = static_cast<llvm::Value*>(m_rhs->codegen(ctx));
    assertNonNull(lhs);
    assertNonNull(rhs);
    switch (m_op) {
        case ast::BinaryOpType::add: {
            return ctx->builder->CreateAdd(lhs, rhs, "addtmp");
        }
        case ast::BinaryOpType::sub: {
            return ctx->builder->CreateSub(lhs, rhs, "subtmp");
        }
        case ast::BinaryOpType::mul: {
            return ctx->builder->CreateMul(lhs, rhs, "multmp");
        }
        case ast::BinaryOpType::div: {
            return ctx->builder->CreateUDiv(lhs, rhs, "divtmp");
        }
        case ast::BinaryOpType::mod: {
            return ctx->builder->CreateURem(lhs, rhs, "modulotmp");
        }
        case ast::BinaryOpType::invalid: {
            throw codegen::CodeGenException("encountered an invalid binary operation", m_loc);
        }
        default:
            throw std::runtime_error("invalid value of enum class BinaryOpType: " + std::to_string(static_cast<uint32_t>(m_op)));
    }
    return nullptr;
}

void *ast::UnaryOp::codegen(void *ctx_) const {
    codegen::Context *ctx = static_cast<codegen::Context*>(ctx_);
    llvm::Value *rhs = static_cast<llvm::Value*>(m_rhs->codegen(ctx));
    assertNonNull(rhs);
    switch (m_op) {
        case ast::UnaryOpType::neg: {
            return ctx->builder->CreateSub(llvm::ConstantInt::get(*ctx->llvm_ctx, llvm::APInt(8, 0, false)), rhs, "negtmp");
        }
        case ast::UnaryOpType::invalid: {
            throw codegen::CodeGenException("encountered an invalid unary operation", m_loc);
        }
        default:
            throw std::runtime_error("invalid value of enum class BinaryOpType: " + std::to_string(static_cast<uint32_t>(m_op)));
    }
    return nullptr;
}

void *ast::VarRef::codegen(void *ctx_) const {
    codegen::Context *ctx = static_cast<codegen::Context*>(ctx_);
    if (!variableDefined(ctx, m_name))
        throw codegen::CodeGenException(std::string("use of undeclared variable '") + m_name + "'", m_loc);
    llvm::AllocaInst *var_ptr = ctx->state->named_values.at(m_name);
    return ctx->builder->CreateLoad(ctx->builder->getInt8Ty(), var_ptr, m_name + ".loadtmp");
}

void *ast::Constant::codegen(void *ctx_) const {
    codegen::Context *ctx = static_cast<codegen::Context*>(ctx_);
    return llvm::ConstantInt::get(*ctx->llvm_ctx, llvm::APInt(8, m_value, false));
}

void *ast::FunctionCall::codegen(void *ctx_) const {
    codegen::Context *ctx = static_cast<codegen::Context*>(ctx_);
    llvm::Function *callee = ctx->module->getFunction(m_name);
    if (!callee) {
        std::vector<std::string> arg_names(m_args.size(), std::string("arg"));
        auto proto = ast::FunctionProto {
            .name = m_name,
            .args = arg_names,
        };
        createPrototype(ctx, &proto);
    }
    callee = ctx->module->getFunction(m_name);
    if (!callee)
        throw codegen::CodeGenException(std::string("failed to generate prototype for function '") + m_name + "'", m_loc);
    if (callee->arg_size() != m_args.size())
        throw codegen::CodeGenException("incorrect function signature for function '" + m_name + "': function takes "
                               + std::to_string(callee->arg_size()) + " args, not " + std::to_string(m_args.size()), m_loc);
    std::vector<llvm::Value*> args;
    for (uint32_t i = 0; i < m_args.size(); i++)
        args.push_back(static_cast<llvm::Value*>(m_args.at(i)->codegen(ctx)));
    return ctx->builder->CreateCall(callee, std::move(args), "calltmp");
}

void createLifetimeCall(codegen::Context *ctx, llvm::Value *obj, llvm::BasicBlock *lifetime_bb, char const *instruct_name) {
    // TODO currently the lifetime start calls are inserted in an extra block, but this behavior is only necessary for lifetime end calls.
    // The lifetime start calls could instead be inserted right before the decl assignment
    assertNonNull(obj);
    std::vector<llvm::Type*> lifetime_intrinsics_args({ctx->builder->getInt64Ty(), llvm::PointerType::getUnqual(ctx->builder->getInt8Ty())});
    llvm::FunctionCallee fc = ctx->module->getOrInsertFunction(instruct_name, llvm::FunctionType::get(ctx->builder->getVoidTy(), lifetime_intrinsics_args, false));
    llvm::Function *func = llvm::dyn_cast<llvm::Function>(fc.getCallee());
    auto type_size = llvm::ConstantInt::get(ctx->builder->getInt64Ty(), ctx->module->getDataLayout().getTypeAllocSize(obj->getType()));
    auto saved_ip = ctx->builder->saveIP();
    ctx->builder->SetInsertPoint(lifetime_bb);
    ctx->builder->CreateCall(func, {type_size, obj});
    ctx->builder->restoreIP(saved_ip);
}

void createLifetimeStartCall(codegen::Context *ctx, llvm::Value *obj, llvm::BasicBlock *lifetime_bb) {
    createLifetimeCall(ctx, obj, lifetime_bb, "llvm.lifetime.start");
}

void createLifetimeEndCall(codegen::Context *ctx, llvm::Value *obj, llvm::BasicBlock *lifetime_bb) {
    createLifetimeCall(ctx, obj, lifetime_bb, "llvm.lifetime.end");
}

void *ast::Block::codegen(void *ctx_) const {
    codegen::Context *ctx = static_cast<codegen::Context*>(ctx_);
    codegen::State *old_state_ptr = ctx->state;
    auto new_state = codegen::State {
        .named_values = old_state_ptr->named_values,
        .declarations_block = old_state_ptr->declarations_block,
    };
    ctx->state = &new_state;
    if (m_is_toplevel) {
        for (auto const &stmt : m_statements) {
            if (stmt->getKind() == ast::StatementKind::function_def)
                createPrototype(ctx, &stmt->getProto());
        }
        for (auto const &stmt : m_statements) {
            if (stmt->getKind() == ast::StatementKind::function_def) {
                try {
                    stmt->codegen(ctx);
                } catch (codegen::CodeGenException e) {
                    ctx->state = &new_state;
                    ctx->errors->push_back(codegen::Error {
                        .loc = e.m_loc,
                        .msg = std::move(e.m_message),
                    });
                }
            } else if (stmt->getKind() == ast::StatementKind::decl_assignment) {
                throw std::runtime_error("TODO: implement global variables");
            } else
                throw std::runtime_error("parser generated other statement type in toplevel even though it should only generate function defs and decl assignments");
        }
        ctx->state = old_state_ptr;
        llvm::verifyModule(*ctx->module);
        return nullptr;
    } else {
        llvm::Function *parent_fn = ctx->builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *decl_lifetime_start_bb = llvm::BasicBlock::Create(*ctx->llvm_ctx, "block_lifetimes_start", parent_fn);
        llvm::BasicBlock *block_entry_bb = llvm::BasicBlock::Create(*ctx->llvm_ctx, "block_entry", parent_fn);
        llvm::BasicBlock *decl_lifetime_end_bb = llvm::BasicBlock::Create(*ctx->llvm_ctx, "block_lifetimes_end");
        ctx->builder->CreateBr(decl_lifetime_start_bb);
        ctx->builder->SetInsertPoint(block_entry_bb);

        std::vector<llvm::Value*> alloca_ptrs_of_decls;
        for (auto const &stmt : m_statements) {
            if (stmt->getKind() == ast::StatementKind::decl_assignment) {
                llvm::Value* var = nullptr;
                try {
                    llvm::Value *var = static_cast<llvm::Value*>(stmt->codegen(ctx));
                    assertNonNull(var);
                    createLifetimeStartCall(ctx, var, decl_lifetime_start_bb);
                    alloca_ptrs_of_decls.push_back(var);
                } catch (codegen::CodeGenException e) {
                    // recover state in case it was not reset back before error (which will always happen atm)
                    ctx->state = &new_state;
                    ctx->errors->push_back(codegen::Error {
                        .loc = e.m_loc,
                        .msg = std::move(e.m_message),
                    });
                    alloca_ptrs_of_decls.push_back(nullptr);
                }
            } else if (stmt->getKind() == ast::StatementKind::function_def)
                throw std::runtime_error("parser accepted and constructed a function def in a non-toplevel scope");
            else {
                try {
                    stmt->codegen(ctx);
                } catch (codegen::CodeGenException e) {
                    ctx->state = &new_state;
                    ctx->errors->push_back(codegen::Error {
                        .loc = e.m_loc,
                        .msg = std::move(e.m_message),
                    });
                }
                alloca_ptrs_of_decls.push_back(nullptr);
            }
        }

        llvm::Value *result = nullptr;
        if (m_result) {
            try {
                result = static_cast<llvm::Value*>(m_result.value()->codegen(ctx));
            } catch (codegen::CodeGenException e) {
                ctx->errors->push_back(codegen::Error {
                    .loc = e.m_loc,
                    .msg = std::move(e.m_message),
                });
            }
        }

        auto saved_ip = ctx->builder->saveIP();
        ctx->builder->SetInsertPoint(decl_lifetime_start_bb);
        ctx->builder->CreateBr(block_entry_bb);
        ctx->builder->restoreIP(saved_ip);
        ctx->builder->CreateBr(decl_lifetime_end_bb);
        parent_fn->insert(parent_fn->end(), decl_lifetime_end_bb);
        ctx->builder->SetInsertPoint(decl_lifetime_end_bb);
        for (auto const &alloca_ptr : alloca_ptrs_of_decls) {
            if (alloca_ptr != nullptr)
                createLifetimeEndCall(ctx, alloca_ptr, decl_lifetime_end_bb);
        }

        ctx->state = old_state_ptr;
        return result;
    }
}

void *ast::If::codegen(void *ctx_) const {
    codegen::Context *ctx = static_cast<codegen::Context*>(ctx_);
    llvm::Value *condition = ctx->builder->CreateICmpNE(static_cast<llvm::Value*>(m_condition->codegen(ctx)), llvm::ConstantInt::get(*ctx->llvm_ctx, llvm::APInt(8, 0, false)), "condtmp");
    llvm::Function *parent_fn = ctx->builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *cond_true_bb = llvm::BasicBlock::Create(*ctx->llvm_ctx, "cond_true", parent_fn);
    llvm::BasicBlock *cond_false_bb = llvm::BasicBlock::Create(*ctx->llvm_ctx, "cond_false");
    llvm::BasicBlock *post_if_bb = llvm::BasicBlock::Create(*ctx->llvm_ctx, "post_if");

    // create conditional branch
    llvm::AllocaInst *if_result = allocaInDeclBlock(ctx, ctx->builder->getInt8Ty(), "if_result");
    ctx->builder->CreateCondBr(condition, cond_true_bb, cond_false_bb);

    // condition true branch
    ctx->builder->SetInsertPoint(cond_true_bb);
    llvm::Value *cond_true_result = static_cast<llvm::Value*>(m_branch->codegen(ctx));

    if (cond_true_result)
        ctx->builder->CreateStore(cond_true_result, if_result);
    else
        ctx->builder->CreateStore(llvm::ConstantInt::get(*ctx->llvm_ctx, llvm::APInt(8, 0, false)), if_result);
    ctx->builder->CreateBr(post_if_bb);
    cond_true_bb = ctx->builder->GetInsertBlock();

    // condition false branch
    parent_fn->insert(parent_fn->end(), cond_false_bb);
    ctx->builder->SetInsertPoint(cond_false_bb);
    llvm::Value *cond_false_result = nullptr;
    if (m_else_branch)
        cond_false_result = static_cast<llvm::Value*>(m_else_branch.value()->codegen(ctx));

    if (cond_true_result != nullptr || cond_false_result != nullptr) {
        std::optional<std::string> message = std::nullopt;
        if (cond_true_result == nullptr && cond_false_result != nullptr)
            message = std::string("incompatible result types of true and false branch of if condition; true branch type: void; false branch type: ")
                + llvmTypeAsString(cond_false_result->getType());
        else if (cond_true_result != nullptr && cond_false_result == nullptr)
            message = std::string("incompatible result types of true and false branch of if condition; true branch type: ")
                + llvmTypeAsString(cond_true_result->getType())
                + "; false branch type: void";
        else if (cond_true_result->getType() != cond_false_result->getType())
            message = std::string("incompatible result types of true and false branch of if condition; true branch type: ")
                + llvmTypeAsString(cond_true_result->getType())
                + "; false branch type: "
                + llvmTypeAsString(cond_false_result->getType());
        if (message)
            ctx->warnings->push_back(codegen::Warning {
                .loc = m_loc,
                .msg = std::move(message.value()),
            });
    }

    if (cond_false_result)
        ctx->builder->CreateStore(cond_false_result, if_result);
    else
        ctx->builder->CreateStore(llvm::ConstantInt::get(*ctx->llvm_ctx, llvm::APInt(8, 0, false)), if_result);
    ctx->builder->CreateBr(post_if_bb);
    cond_false_bb = ctx->builder->GetInsertBlock();

    // post if block (where codegen continues)
    parent_fn->insert(parent_fn->end(), post_if_bb);
    ctx->builder->SetInsertPoint(post_if_bb);
    llvm::Value *loaded_result = ctx->builder->CreateLoad(ctx->builder->getInt8Ty(), if_result, "if_result.loadtmp");
    return loaded_result;
}

void *ast::While::codegen(void *ctx_) const {
    codegen::Context *ctx = static_cast<codegen::Context*>(ctx_);
    llvm::Function *parent_fn = ctx->builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *cond_bb = llvm::BasicBlock::Create(*ctx->llvm_ctx, "cond_block", parent_fn);
    llvm::BasicBlock *loop_body_bb = llvm::BasicBlock::Create(*ctx->llvm_ctx, "loop_body");
    llvm::BasicBlock *post_while_bb = llvm::BasicBlock::Create(*ctx->llvm_ctx, "post_while");

    // create condition block
    // TODO support implicit returns from breaks
    ctx->builder->CreateBr(cond_bb);
    ctx->builder->SetInsertPoint(cond_bb);
    llvm::Value *condition = ctx->builder->CreateICmpNE(static_cast<llvm::Value*>(m_condition->codegen(ctx)), llvm::ConstantInt::get(*ctx->llvm_ctx, llvm::APInt(8, 0, false)));
    ctx->builder->CreateCondBr(condition, loop_body_bb, post_while_bb);

    // loop body branch
    parent_fn->insert(parent_fn->end(), loop_body_bb);
    ctx->builder->SetInsertPoint(loop_body_bb);
    llvm::Value *cond_true_result = static_cast<llvm::Value*>(m_branch->codegen(ctx));
    if (cond_true_result != nullptr)
        throw std::runtime_error("return values from loops not supported at the moment");
    ctx->builder->CreateBr(cond_bb);
    loop_body_bb = ctx->builder->GetInsertBlock();

    // after the loop
    parent_fn->insert(parent_fn->end(), post_while_bb);
    ctx->builder->SetInsertPoint(post_while_bb);
    return nullptr;
}

void *ast::FunctionDef::codegen(void *ctx_) const {
    codegen::Context *ctx = static_cast<codegen::Context*>(ctx_);
    llvm::Function *fn = ctx->module->getFunction(m_proto.name);
    if (!fn)
        throw std::runtime_error("no forward declaration has been auto-generated for this function");
    else if (!fn->empty())
        throw codegen::CodeGenException(std::string("redefinition of function '") + m_proto.name + "'", m_loc);
    llvm::BasicBlock *declarations_bb = llvm::BasicBlock::Create(*ctx->llvm_ctx, "declarations_block", fn);
    llvm::BasicBlock *entry_bb = llvm::BasicBlock::Create(*ctx->llvm_ctx, "entry");
    ctx->builder->SetInsertPoint(declarations_bb);

    codegen::State *old_state_ptr = ctx->state;
    auto new_state = codegen::State {
        .named_values = ctx->state->named_values,  // TODO avoid copies
        .declarations_block = declarations_bb,
    };
    ctx->state = &new_state;

    uint32_t i = 0;
    for (llvm::Argument const &arg_val : fn->args()) {
        llvm::AllocaInst *alloca = allocaInDeclBlock(ctx, ctx->builder->getInt8Ty(), arg_val.getName().data());
        new_state.named_values[m_proto.args[i++]] = alloca;
    }
    
    fn->insert(fn->end(), entry_bb);
    ctx->builder->SetInsertPoint(entry_bb);
    llvm::Value *implicit_ret = static_cast<llvm::Value*>(m_block->codegen(ctx));
    if (implicit_ret)
        ctx->builder->CreateRet(implicit_ret);
    else
        ctx->builder->CreateRet(llvm::ConstantInt::get(*ctx->llvm_ctx, llvm::APInt(8, 0, false)));
    ctx->builder->SetInsertPoint(declarations_bb);
    ctx->builder->CreateBr(entry_bb);
    ctx->state = old_state_ptr;
    // if (!ctx->errors->size())
    llvm::verifyFunction(*fn);
    return nullptr;
}

void *ast::DeclAssignment::global_codegen(void *ctx_) const {
    codegen::Context *ctx = static_cast<codegen::Context*>(ctx_);
    throw std::runtime_error("global variables are hard :( I'll figure them out later");
}

void *ast::DeclAssignment::codegen(void *ctx_) const {
    codegen::Context *ctx = static_cast<codegen::Context*>(ctx_);
    llvm::AllocaInst *var = allocaInDeclBlock(ctx, ctx->builder->getInt8Ty(), m_name.c_str());
    if (m_value) {
        llvm::Value *value = static_cast<llvm::Value*>(m_value.value()->codegen(ctx));
        ctx->builder->CreateStore(value, var);
    }
    ctx->state->named_values[m_name] = var;
    return static_cast<llvm::Value*>(var);
}

void *ast::Assignment::codegen(void *ctx_) const {
    codegen::Context *ctx = static_cast<codegen::Context*>(ctx_);
    llvm::Value *value = static_cast<llvm::Value*>(m_value->codegen(ctx));

    if (m_key->getKind() == ast::ExprKind::var_ref) {
        std::string const &name = m_key->getVarName();
        if (variableDefined(ctx, name)) {
            llvm::AllocaInst *var = ctx->state->named_values.at(name);
            ctx->builder->CreateStore(value, var);
        } else  // TODO support pointer deref assignments here
            throw codegen::CodeGenException(std::string("use of undeclared variable '") + name + "'", m_loc);
    } else
        throw codegen::CodeGenException("invalid lhs for assignment: lhs must be either an identifier (or in the future, a dereference of some expression)", m_loc);
    return nullptr;
}

void *ast::Return::codegen(void *ctx_) const {
    codegen::Context *ctx = static_cast<codegen::Context*>(ctx_);
    llvm::Value *value = static_cast<llvm::Value*>(m_value->codegen(ctx));
    ctx->builder->CreateRet(value);
    return nullptr;
}

void *ast::ExprStmt::codegen(void *ctx_) const {
    codegen::Context *ctx = static_cast<codegen::Context*>(ctx_);
    llvm::Value *expr = static_cast<llvm::Value*>(m_expr->codegen(ctx));
    return nullptr;
}
