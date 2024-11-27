#include "ast.hpp"

ast::Expr::Expr(LocationInfo loc) : m_loc(loc)
{}

std::string ast::Expr::toJsonString() const {
    throw std::runtime_error("called toJsonString on abstract type ast::Expr");
}

ast::ExprKind ast::Expr::getKind() const {
    return ast::ExprKind::abstract_expr_type;
}

std::string const &ast::Expr::getVarName() const {
    throw std::runtime_error("called getVarName on non-ident ast node");
}

ast::Statement::Statement(LocationInfo loc) : m_loc(loc)
{}

std::string ast::Statement::toJsonString() const {
    throw std::runtime_error("called toJsonString on abstract type ast::Statement");
}

ast::StatementKind ast::Statement::getKind() const {
    return ast::StatementKind::abstract_statement_type;
}

ast::FunctionProto const &ast::Statement::getProto() const {
    throw std::runtime_error("called getProto on non-function-def ast node");
}

#define BOTFFTT_MAP(kind, mapped) if (t == token::TokenType::kind) return BinaryOpType::mapped
#define UOTFFTT_MAP(kind, mapped) if (t == token::TokenType::kind) return UnaryOpType::mapped

ast::BinaryOpType ast::binaryOpTypeFromTokenType(token::TokenType t) {
    BOTFFTT_MAP(plus, add);
    BOTFFTT_MAP(minus, sub);
    BOTFFTT_MAP(asterisk, mul);
    BOTFFTT_MAP(slash, div);
    BOTFFTT_MAP(percent, mod);
    return BinaryOpType::invalid;
}

ast::UnaryOpType ast::unaryOpTypeFromTokenType(token::TokenType t) {
    UOTFFTT_MAP(minus, neg);
    return UnaryOpType::invalid;
}

std::string ast::binaryOpTypeToString(ast::BinaryOpType t) {
    if (t == ast::BinaryOpType::add) return "add";
    if (t == ast::BinaryOpType::sub) return "sub";
    if (t == ast::BinaryOpType::mul) return "mul";
    if (t == ast::BinaryOpType::div) return "div";
    if (t == ast::BinaryOpType::mod) return "mod";
    return "invalid";
}

std::string ast::unaryOpTypeToString(ast::UnaryOpType t) {
    if (t == ast::UnaryOpType::neg) return "neg";
    return "invalid";
}

std::string jsonLocPrefix(LocationInfo loc) {
    return std::string("{\"line\": ") + std::to_string(loc.line) + ", \"file\": \"" + loc.file.start + "\", ";
}

ast::BinaryOp::BinaryOp(
    LocationInfo loc,
    std::unique_ptr<Expr> lhs,
    std::unique_ptr<Expr> rhs,
    ast::BinaryOpType op
) : ast::Expr(loc), m_lhs(std::move(lhs)), m_rhs(std::move(rhs)), m_op(op)
{}

std::string ast::BinaryOp::toJsonString() const {
    return jsonLocPrefix(m_loc) + "\"kind\": \"binary_op\", \"op\": \""
        + ast::binaryOpTypeToString(m_op)
        + "\", \"lhs\": "
        + m_lhs->toJsonString()
        + ", \"rhs\": "
        + m_rhs->toJsonString()
        + "}";
}

ast::ExprKind ast::BinaryOp::getKind() const {
    return ast::ExprKind::binary_op;
}

ast::UnaryOp::UnaryOp(
    LocationInfo loc,
    std::unique_ptr<Expr> rhs,
    ast::UnaryOpType op
) : ast::Expr(loc), m_rhs(std::move(rhs)), m_op(op)
{}

std::string ast::UnaryOp::toJsonString() const {
    return jsonLocPrefix(m_loc)
        + "\"kind\": \"unary_op\", \"op\": \""
        + ast::unaryOpTypeToString(m_op)
        + "\", \"rhs\": "
        + m_rhs->toJsonString()
        + "}";
}

ast::ExprKind ast::UnaryOp::getKind() const {
    return ast::ExprKind::unary_op;
}

ast::VarRef::VarRef(
    LocationInfo loc,
    std::string name
) : ast::Expr(loc), m_name(std::move(name))
{}

std::string ast::VarRef::toJsonString() const {
    return jsonLocPrefix(m_loc) + "\"kind\": \"var_ref\", \"name\": \"" + m_name + "\"}";
}

ast::ExprKind ast::VarRef::getKind() const {
    return ast::ExprKind::var_ref;
}

std::string const &ast::VarRef::getVarName() const {
    return m_name;
}

ast::Constant::Constant(
    LocationInfo loc,
    uint64_t value
) : ast::Expr(loc), m_value(value)
{}

std::string ast::Constant::toJsonString() const {
    return jsonLocPrefix(m_loc) + "\"kind\": \"constant\", \"value\": " + std::to_string(m_value) + "}";
}

ast::ExprKind ast::Constant::getKind() const {
    return ast::ExprKind::constant;
}

ast::FunctionCall::FunctionCall(
    LocationInfo loc,
    std::string name,
    std::vector<std::unique_ptr<Expr>> args
) : ast::Expr(loc), m_name(std::move(name)), m_args(std::move(args))
{}

std::string ast::FunctionCall::toJsonString() const {
    std::string result = jsonLocPrefix(m_loc) + "\"kind\": \"function_call\", \"name\": \"" + m_name + "\", \"args\": [";
    for (uint32_t i = 0; i < m_args.size(); i++) {
        result += m_args[i]->toJsonString();
        if (i != m_args.size() - 1)
            result += ", ";
    }
    return result + "]}";
}

ast::ExprKind ast::FunctionCall::getKind() const {
    return ast::ExprKind::function_call;
}

ast::Block::Block(
    LocationInfo loc,
    std::vector<std::unique_ptr<Statement>> statements,
    std::optional<std::unique_ptr<Expr>> result,
    bool is_toplevel
) : ast::Expr(loc), m_statements(std::move(statements)), m_result(std::move(result)), m_is_toplevel(is_toplevel)
{}

std::string ast::Block::toJsonString() const {
    std::string result = jsonLocPrefix(m_loc) + "\"kind\": \"block\", \"statements\": [";
    for (uint32_t i = 0; i < m_statements.size(); i++) {
        result += m_statements[i]->toJsonString();
        if (i != m_statements.size() - 1)
            result += ", ";
    }
    result += "], \"result\": ";
    if (m_result)
        result += m_result.value()->toJsonString();
    else
        result += "null";
    return result + "}";
}

ast::ExprKind ast::Block::getKind() const {
    return ast::ExprKind::block;
}

ast::If::If(
    LocationInfo loc,
    std::unique_ptr<Expr> condition,
    std::unique_ptr<Expr> branch,
    std::optional<std::unique_ptr<Expr>> else_branch
) : ast::Expr(loc), m_condition(std::move(condition)), m_branch(std::move(branch)), m_else_branch(std::move(else_branch))
{}

std::string ast::If::toJsonString() const {
    std::string result = jsonLocPrefix(m_loc) + "\"kind\": \"if\", \"condition\": " + m_condition->toJsonString() + ", \"branch\": " + m_branch->toJsonString() + ", \"else_branch\": ";
    if (m_else_branch)
        result += m_else_branch.value()->toJsonString();
    else
        result += "null";
    return result + "}";
}

ast::ExprKind ast::If::getKind() const {
    return ast::ExprKind::if_;
}

ast::While::While(
    LocationInfo loc,
    std::unique_ptr<Expr> condition,
    std::unique_ptr<Expr> branch
) : ast::Expr(loc), m_condition(std::move(condition)), m_branch(std::move(branch))
{}

std::string ast::While::toJsonString() const {
    return jsonLocPrefix(m_loc) + "\"kind\": \"while\", \"condition\": " + m_condition->toJsonString() + ", \"branch\": " + m_branch->toJsonString() + "}";
}

ast::ExprKind ast::While::getKind() const {
    return ast::ExprKind::while_;
}

ast::For::For(
    LocationInfo loc,
    std::unique_ptr<Statement> init,
    std::unique_ptr<Expr> condition,
    std::unique_ptr<Statement> update,
    std::unique_ptr<Expr> branch
) : ast::Expr(loc), m_init(std::move(init)), m_condition(std::move(condition)), m_update(std::move(update)), m_branch(std::move(branch))
{}

std::string ast::For::toJsonString() const {
    return jsonLocPrefix(m_loc)
        + "\"kind\": \"for\", \"init\": "
        + m_branch->toJsonString()
        + ", \"condition\": "
        + m_condition->toJsonString()
        + ", \"update\": "
        + m_update->toJsonString()
        + ", \"branch\": "
        + m_branch->toJsonString()
        + "}";
}

ast::ExprKind ast::For::getKind() const {
    return ast::ExprKind::for_;
}

ast::FunctionDef::FunctionDef(
    LocationInfo loc,
    std::string name,
    std::vector<std::string> args,
    std::unique_ptr<Block> block,
    bool is_extern,
    bool is_fastcc
) : ast::Statement(loc), m_block(std::move(block))
{
    m_proto = ast::FunctionProto {
        .name = std::move(name),
        .args = std::move(args),
        .is_extern = is_extern,
        .is_fastcc = is_fastcc
    };
}

ast::FunctionDef::FunctionDef(
    LocationInfo loc,
    ast::FunctionProto proto,
    std::unique_ptr<Block> block
) : ast::Statement(loc), m_proto(std::move(proto)), m_block(std::move(block))
{}

ast::StatementKind ast::FunctionDef::getKind() const {
    return ast::StatementKind::function_def;
}

std::string ast::FunctionDef::toJsonString() const {
    std::string result = jsonLocPrefix(m_loc) + "\"kind\": \"function_def\", \"proto\": {\"name\": \"" + m_proto.name + "\", \"args\": [";
    for (uint32_t i = 0; i < m_proto.args.size(); i++) {
        result = result + "\"" + m_proto.args[i] + "\"";
        if (i != m_proto.args.size() - 1)
            result += ", ";
    }
    result = result + "]}, \"block\": " + m_block->toJsonString() + "}";
    return result;
}

ast::FunctionProto const &ast::FunctionDef::getProto() const {
    return m_proto;
}

ast::DeclAssignment::DeclAssignment(
    LocationInfo loc,
    std::string name,
    std::optional<std::unique_ptr<Expr>> value
) : ast::Statement(loc), m_name(std::move(name)), m_value(std::move(value))
{}

std::string ast::DeclAssignment::toJsonString() const {
    auto result = jsonLocPrefix(m_loc) + "\"kind\": \"decl_assignment\", \"name\": \"" + m_name + "\", \"value\": ";
    if (m_value)
        result = result + m_value.value()->toJsonString() + "}";
    return result;
}

ast::StatementKind ast::DeclAssignment::getKind() const {
    return ast::StatementKind::decl_assignment;
}

ast::Assignment::Assignment(
    LocationInfo loc,
    std::unique_ptr<Expr> key,
    std::unique_ptr<Expr> value
) : ast::Statement(loc), m_key(std::move(key)), m_value(std::move(value))
{}

std::string ast::Assignment::toJsonString() const {
    return jsonLocPrefix(m_loc) + "\"kind\": \"assignment\", \"key\": " + m_key->toJsonString() + ", \"value\": " + m_value->toJsonString() + "}";
}

ast::StatementKind ast::Assignment::getKind() const {
    return ast::StatementKind::assignment;
}

ast::Return::Return(
    LocationInfo loc,
    std::unique_ptr<Expr> value
) : ast::Statement(loc), m_value(std::move(value))
{}

std::string ast::Return::toJsonString() const {
    return jsonLocPrefix(m_loc) + "\"kind\": \"return\", \"value\": " + m_value->toJsonString() + "}";
}

ast::StatementKind ast::Return::getKind() const {
    return ast::StatementKind::return_;
}

ast::ExprStmt::ExprStmt(
    LocationInfo loc,
    std::unique_ptr<Expr> expr
) : ast::Statement(loc), m_expr(std::move(expr))
{}

std::string ast::ExprStmt::toJsonString() const {
    return jsonLocPrefix(m_loc) + "\"kind\": \"expr_stmt\", \"expr\": " + m_expr->toJsonString() + "}";
}

ast::StatementKind ast::ExprStmt::getKind() const {
    return ast::StatementKind::expr_stmt;
}
