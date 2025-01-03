#pragma once

#include "lexer.hpp"
#include "lib.hpp"
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

namespace ast {
typedef enum class ExprKind {
    abstract_expr_type,
    unary_op,
    binary_op,
    constant,
    var_ref,
    function_call,
    block,
    if_,
    while_,
    for_,
} ExprKind;

typedef enum class StatementKind {
    abstract_statement_type,
    assignment,
    decl_assignment,
    function_def,
    return_,
    expr_stmt,
} StatementKind;

typedef struct FunctionProto {
    std::string name;
    std::vector<std::string> args;
    bool is_extern;
    bool is_fastcc;
} FunctionProto;

class Expr {
protected:
    LocationInfo m_loc;

public:
    Expr(LocationInfo loc);
    virtual std::string toJsonString() const;
    virtual ExprKind getKind() const;
    virtual std::string const &getVarName() const;
    virtual void *codegen(void *ctx_) const;
};

class Statement {
protected:
    LocationInfo m_loc;

public:
    Statement(LocationInfo loc);
    virtual std::string toJsonString() const;
    virtual StatementKind getKind() const;
    virtual FunctionProto const &getProto() const;
    virtual void *codegen(void *ctx_) const;
};

typedef enum class BinaryOpType {
    add,
    sub,
    mul,
    div,
    mod,
    invalid,
    // TODO bitwise ops
} BinaryOpType;

typedef enum class UnaryOpType {
    neg,
    invalid,
    // TODO ref, deref
} UnaryOpType;


BinaryOpType binaryOpTypeFromTokenType(token::TokenType t);
UnaryOpType unaryOpTypeFromTokenType(token::TokenType t);

std::string binaryOpTypeToString(BinaryOpType t);
std::string unaryOpTypeToString(UnaryOpType t);

class BinaryOp : public Expr {
    std::unique_ptr<Expr> m_lhs;
    std::unique_ptr<Expr> m_rhs;
    BinaryOpType m_op;

public:
    BinaryOp(
        LocationInfo loc,
        std::unique_ptr<Expr> lhs,
        std::unique_ptr<Expr> rhs,
        BinaryOpType op
    );
    std::string toJsonString() const override;
    ExprKind getKind() const override;
    void *codegen(void *ctx_) const override;
};

class UnaryOp : public Expr {
    std::unique_ptr<Expr> m_rhs;
    UnaryOpType m_op;

public:
    UnaryOp(LocationInfo loc, std::unique_ptr<Expr> rhs, UnaryOpType op);
    std::string toJsonString() const override;
    ExprKind getKind() const override;
    void *codegen(void *ctx_) const override;
};

class VarRef : public Expr {
    std::string m_name;

public:
    VarRef(LocationInfo loc, std::string name);
    std::string toJsonString() const override;
    ExprKind getKind() const override;
    std::string const &getVarName() const override;
    void *codegen(void *ctx_) const override;
};

class Constant : public Expr {
    uint64_t m_value;

public:
    Constant(LocationInfo loc, uint64_t value);
    std::string toJsonString() const override;
    ExprKind getKind() const override;
    void *codegen(void *ctx_) const override;
};

class FunctionCall : public Expr {
    std::string m_name;
    std::vector<std::unique_ptr<Expr>> m_args;

public:
    FunctionCall(
        LocationInfo loc,
        std::string name,
        std::vector<std::unique_ptr<Expr>> args
    );
    std::string toJsonString() const override;
    ExprKind getKind() const override;
    void *codegen(void *ctx_) const override;
};

class Block : public Expr {
    std::vector<std::unique_ptr<Statement>> m_statements;
    std::optional<std::unique_ptr<Expr>> m_result;
    bool m_is_toplevel;

public:
    Block(
        LocationInfo loc,
        std::vector<std::unique_ptr<Statement>> statements,
        std::optional<std::unique_ptr<Expr>> result,
        bool is_toplevel
    );
    std::string toJsonString() const override;
    ExprKind getKind() const override;
    void *codegen(void *ctx_) const override;
};

class If : public Expr {
    std::unique_ptr<Expr> m_condition;
    std::unique_ptr<Expr> m_branch;
    std::optional<std::unique_ptr<Expr>> m_else_branch;

public:
    If(LocationInfo loc, std::unique_ptr<Expr> condition, std::unique_ptr<Expr> branch, std::optional<std::unique_ptr<Expr>> else_branch);
    std::string toJsonString() const override;
    ExprKind getKind() const override;
    void *codegen(void *ctx_) const override;
};

class While : public Expr {
    std::unique_ptr<Expr> m_condition;
    std::unique_ptr<Expr> m_branch;

public:
    While(LocationInfo loc, std::unique_ptr<Expr> condition, std::unique_ptr<Expr> branch);
    std::string toJsonString() const override;
    ExprKind getKind() const override;
    void *codegen(void *ctx_) const override;
};

class For : public Expr {
    std::unique_ptr<Statement> m_init;
    std::unique_ptr<Expr> m_condition;
    std::unique_ptr<Statement> m_update;
    std::unique_ptr<Expr> m_branch;

public:
    For(LocationInfo loc, std::unique_ptr<Statement> init, std::unique_ptr<Expr> condition, std::unique_ptr<Statement> update, std::unique_ptr<Expr> branch);
    std::string toJsonString() const override;
    ExprKind getKind() const override;
    void *codegen(void *ctx_) const override;
};

class FunctionDef : public Statement {
    FunctionProto m_proto;
    // TODO one could probably get rid of this unique_ptr
    std::unique_ptr<Block> m_block;

public:
    FunctionDef(
        LocationInfo loc,
        std::string name,
        std::vector<std::string> args,
        std::unique_ptr<Block> block,
        bool is_extern = true,
        bool is_fastcc = true
    );
    FunctionDef(
        LocationInfo loc,
        FunctionProto proto,
        std::unique_ptr<Block> block
    );
    std::string toJsonString() const override;
    StatementKind getKind() const override;
    FunctionProto const &getProto() const override;
    void *codegen(void *ctx_) const override;
};

class DeclAssignment : public Statement {
    std::string m_name;
    std::optional<std::unique_ptr<Expr>> m_value;

public:
    DeclAssignment(LocationInfo loc, std::string name, std::optional<std::unique_ptr<Expr>> value);
    std::string toJsonString() const override;
    StatementKind getKind() const override;
    void *codegen(void *ctx_) const override;
    void *globalCodegen(void *ctx_) const;
};

class Assignment : public Statement {
    std::unique_ptr<Expr> m_key;
    std::unique_ptr<Expr> m_value;

public:
    Assignment(LocationInfo loc, std::unique_ptr<Expr> key, std::unique_ptr<Expr> value);
    std::string toJsonString() const override;
    StatementKind getKind() const override;
    void *codegen(void *ctx_) const override;
};

class Return : public Statement {
    std::unique_ptr<Expr> m_value;

public:
    Return(LocationInfo loc, std::unique_ptr<Expr> value);
    std::string toJsonString() const override;
    StatementKind getKind() const override;
    void *codegen(void *ctx_) const override;
};

class ExprStmt : public Statement {
    std::unique_ptr<Expr> m_expr;

public:
    ExprStmt(LocationInfo loc, std::unique_ptr<Expr> expr);
    std::string toJsonString() const override;
    StatementKind getKind() const override;
    void *codegen(void *ctx_) const override;
};
}  // namespace ast
