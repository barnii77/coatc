#pragma once

#include "lexer.hpp"
#include "ast.hpp"
#include "lib.hpp"
#include <cstdint>
#include <stack>
#include <algorithm>
#include <optional>
#include <stdexcept>
#include <vector>
#include <string>
#include <memory>

namespace parser {
// TODO add a required LocationInfo field to this
class UnexpectedTokenError : public std::exception {
    std::string m_message;

public:
    UnexpectedTokenError(std::string const &message, std::optional<token::Token> const &unexpected_token, char const *note = nullptr);
    char const *what();
    std::string const &getMessage();
};

typedef struct TokenInfo {
    token::Token const *start;
    uint32_t length;
} TokenInfo;

typedef struct TokenIter {
    token::Token const *tokens;
    uint32_t n_remain;

    /// consume next token and return
    std::optional<token::Token> next();
    /// return next token without consuming
    std::optional<token::Token> peek() const;
    /// return nth next token without consuming
    std::optional<token::Token> peek(uint32_t n) const;
    /// return last item
    std::optional<token::Token> peekLast() const;
} TokenIter;

// TODO replace line and file by LocationInfo loc field
typedef struct Error {
    LocationInfo loc;
    std::string msg;
} Error;

typedef struct ParseState {
    TokenInfo info;
    TokenIter iter;
    std::vector<Error> *errors;
    StringRef file;

    /// consume next token and return
    std::optional<token::Token> next();
    /// return next token without consuming
    std::optional<token::Token> peek() const;
    /// return nth next token without consuming
    std::optional<token::Token> peek(uint32_t n) const;
    /// return last item
    std::optional<token::Token> peekLast() const;
    struct ParseState clone() const;
} ParseState;

// TODO do I actually need to export all of these?
std::unique_ptr<ast::Expr> parseExpression(ParseState *ps);
std::unique_ptr<ast::If> parseIfCond(ParseState *ps);
std::unique_ptr<ast::While> parseWhileLoop(ParseState *ps);
std::unique_ptr<ast::FunctionCall> parseFunctionCall(ParseState *ps);
// std::unique_ptr<ast::Assignment> parseAssignment(ParseState *ps);  // replaced for efficiency reasons by direct parsing in parseStatement
std::unique_ptr<ast::DeclAssignment> parseDeclAssignment(ParseState *ps);
std::unique_ptr<ast::Return> parseReturn(ParseState *ps);
std::unique_ptr<ast::FunctionDef> parseFunctionDef(ParseState *ps);
std::unique_ptr<ast::Statement> parseStatement(ParseState *ps, bool is_toplevel = false);
std::unique_ptr<ast::Block> parseBlock(ParseState *ps, bool is_toplevel = false, bool allow_implicit_return = true);
std::unique_ptr<ast::Block> parse(StringRef file, std::vector<token::Token> const &tokens, std::vector<Error> *errors);
}  // namespace parser
