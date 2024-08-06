#include "parser.hpp"

namespace parser {
std::vector<std::vector<token::TokenType>> operators_by_prec = {
    {token::TokenType::asterisk, token::TokenType::slash, token::TokenType::percent},
    {token::TokenType::plus, token::TokenType::minus}
};

std::vector<token::TokenType> const operators = {
    token::TokenType::asterisk,
    token::TokenType::slash,
    token::TokenType::percent,
    token::TokenType::plus,
    token::TokenType::minus
};

std::vector<token::TokenType> const puncts = {
    token::TokenType::comma,
    token::TokenType::semicolon,
    token::TokenType::equals,
    token::TokenType::right_paren,
    token::TokenType::right_brace
};

std::vector<token::TokenType> createEndOfBlockNonExpressionBreakers() {
    std::vector<token::TokenType> result = {
        token::TokenType::else_kwd,
    };
    result.insert(result.end(), operators.begin(), operators.end());
    result.insert(result.end(), puncts.begin(), puncts.end());
    return result;
}

std::vector<token::TokenType> const end_of_block_non_expression_breakers = createEndOfBlockNonExpressionBreakers();

UnexpectedTokenError::UnexpectedTokenError(std::string const &message, std::optional<token::Token> const &unexpected_token) {
    if (unexpected_token)
        m_message = std::string("unexpected token of type ")
            + token::displayTokenType(unexpected_token.value().type)
            + "; info: "
            + message;
    else
        m_message = std::string("ran out of tokens unexpectedly; info: ")
            + message;
}

std::string const &UnexpectedTokenError::getMessage() {
    return m_message;
}

char const *UnexpectedTokenError::what() {
    return m_message.c_str();
}

// consume next token and return
std::optional<token::Token> TokenIter::next() {
    if (n_remain == 0) return std::nullopt;
    n_remain--;
    return *(tokens++);
}

// return next token without consuming
std::optional<token::Token> TokenIter::peek() const {
    if (n_remain == 0) return std::nullopt;
    return *tokens;
}

std::optional<token::Token> TokenIter::peek(uint32_t n) const {
    if (n_remain <= n) return std::nullopt;
    return tokens[n];
}

std::optional<token::Token> TokenIter::peekLast() const {
    if (n_remain)
        return tokens[n_remain - 1];
    return std::nullopt;
}

std::optional<token::Token> ParseState::next() {
    return iter.next();
}

std::optional<token::Token> ParseState::peek() const {
    return iter.peek();
}

std::optional<token::Token> ParseState::peek(uint32_t n) const {
    return iter.peek(n);
}

std::optional<token::Token> ParseState::peekLast() const {
    return iter.peekLast();
}

struct ParseState ParseState::clone() const {
    return ParseState {
        .info = info,
        .iter = iter,
        .errors = errors,
        .file = file
    };
}

token::Token expect(token::TokenType expected_type, std::optional<token::Token> tok) {
    if (!tok) 
        throw UnexpectedTokenError(std::string("expected token of type ") + token::displayTokenType(expected_type) + ", but got nullopt", tok);
    if (tok.value().type == expected_type)
        return tok.value();
    throw UnexpectedTokenError(std::string("expected type ") + token::displayTokenType(expected_type), tok.value());
}

token::Token expectOneOf(std::vector<token::TokenType> expected_types, std::optional<token::Token> tok) {
    if (!tok) 
        throw UnexpectedTokenError(std::string("expected token of one of some types, but got nullopt"), tok);
    if (std::count(expected_types.begin(), expected_types.end(), tok.value().type))
        return tok.value();
    std::string msg = "expected token of one of types { ";
    std::string prefix = "\"";
    for (auto const &expected_type : expected_types)
        msg += prefix + token::displayTokenType(expected_type) + "\",";
    throw UnexpectedTokenError(msg + " }", tok.value());
}

token::Token expectNot(token::TokenType unexpected_type, std::optional<token::Token> tok) {
    if (!tok) 
        throw UnexpectedTokenError(std::string("expected token **not** of type ") + token::displayTokenType(unexpected_type) + ", but got nullopt", tok);
    if (tok.value().type != unexpected_type)
        return tok.value();
    throw UnexpectedTokenError(std::string("did not expect type ") + token::displayTokenType(unexpected_type), tok.value());
}

token::Token expectNoneOf(std::vector<token::TokenType> unexpected_types, std::optional<token::Token> tok) {
    if (!tok) 
        throw UnexpectedTokenError(std::string("expected token **not** of one of some types, but got nullopt"), tok);
    if (std::count(unexpected_types.begin(), unexpected_types.end(), tok.value().type) == 0)
        return tok.value();
    std::string msg = "did not expected token of one of types {";
    std::string prefix = "\"";
    for (auto const &unexpected_type : unexpected_types)
        msg += prefix + token::displayTokenType(unexpected_type) + "\",";
    throw UnexpectedTokenError(msg + "}", tok.value());
}

/// basically just a .value that raises my own custom error and is thus caught
token::Token expectSome(std::optional<token::Token> tok) {
    if (!tok)
        throw UnexpectedTokenError(std::string("expected token, but got nullopt"), tok);
    return tok.value();
}

void unexpectedEof(StringRef file) {
    token::Token eof_tok = {
        .value = StringRef {
            .start = 0,
            .length = 0,
        },
        .type = token::TokenType::eof,
        .meta = std::nullopt,
        .loc = LocationInfo {
            .line = 0,
            .file = file,
        },
    };
    throw UnexpectedTokenError(std::string("unexpected end of file"), eof_tok);
}

typedef struct SplitIterAtTTOut {
    ParseState before;
    ParseState after;
} SplitIterAtTTOut;

SplitIterAtTTOut splitIterAtTT(token::TokenType delimit_type, ParseState *in_ps) {
    std::vector<token::Token> paren_stack;
    auto ps = in_ps->clone();
    uint32_t i = 0;
    for (std::optional<token::Token> tok_; tok_ = ps.peek(); ps.next()) {
        auto tok = tok_.value();
        token::TokenType ty = tok.type;

        if (paren_stack.empty() && ty == delimit_type) {
            break;
        }

        if (ty == token::TokenType::left_paren || ty == token::TokenType::left_brace) {
            paren_stack.push_back(tok);
        } else if (ty == token::TokenType::right_paren) {
            std::optional<token::Token> top = std::nullopt;
            if (!paren_stack.empty()) {
                top = paren_stack.back();
                paren_stack.pop_back();
            }
            expect(token::TokenType::left_paren, top);
        } else if (ty == token::TokenType::right_brace) {
            std::optional<token::Token> top = std::nullopt;
            if (!paren_stack.empty()) {
                top = paren_stack.back();
                paren_stack.pop_back();
            }
            expect(token::TokenType::left_brace, top);
        }
    }
    auto before_ps = in_ps->clone();
    before_ps.iter.n_remain = in_ps->iter.n_remain - ps.iter.n_remain;
    SplitIterAtTTOut out = {
        .before = before_ps,
        .after = ps
    };
    return out;
}

ParseState splitIterAtTTInplace(token::TokenType delimit_type, ParseState *in_ps) {
    std::vector<token::Token> paren_stack;
    auto ps = in_ps->clone();
    uint32_t i = 0;
    for (std::optional<token::Token> tok_; tok_ = in_ps->peek(); in_ps->next()) {
        auto tok = tok_.value();
        token::TokenType ty = tok.type;

        if (paren_stack.empty() && ty == delimit_type) {
            break;
        }

        if (ty == token::TokenType::left_paren || ty == token::TokenType::left_brace) {
            paren_stack.push_back(tok);
        } else if (ty == token::TokenType::right_paren) {
            std::optional<token::Token> top = std::nullopt;
            if (!paren_stack.empty()) {
                top = paren_stack.back();
                paren_stack.pop_back();
            }
            expect(token::TokenType::left_paren, top);
        } else if (ty == token::TokenType::right_brace) {
            std::optional<token::Token> top = std::nullopt;
            if (!paren_stack.empty()) {
                top = paren_stack.back();
                paren_stack.pop_back();
            }
            expect(token::TokenType::left_brace, top);
        }
    }
    ps.iter.n_remain -= in_ps->iter.n_remain;
    return ps;
}

typedef struct ExpressionParsingNodeInfo {
    uint32_t idx;
    bool is_operator;
    LocationInfo loc;
} EPNI;

std::unique_ptr<ast::Expr> parseExpression(ParseState *ps) {
    std::vector<ParseState> parse_states;
    std::vector<EPNI> epnis;
    token::Token const *operand_start = ps->iter.tokens;
    token::Token const *const first_tok = ps->iter.tokens;
    std::vector<token::Token> paren_stack;
    bool last_was_operator = true;
    auto last_ty = token::TokenType::invalid;
    bool entirely_wrapped_in_parens = true;

    for (std::optional<token::Token> tok_; tok_ = ps->peek(); ps->next()) {
        auto tok = tok_.value();  // this doesn't have to be checked because loop condition
        token::TokenType ty = tok.type;

        if (paren_stack.empty()
            && last_ty == token::TokenType::right_brace 
            && std::count(
                end_of_block_non_expression_breakers.begin(),
                end_of_block_non_expression_breakers.end(),
                ty
            ) == 0
        )
            break;
        if (ty == token::TokenType::left_paren || ty == token::TokenType::left_brace) {
            paren_stack.push_back(tok);
            last_was_operator = false;
            last_ty = ty;
            continue;
        } else if (ty == token::TokenType::right_paren) {
            std::optional<token::Token> top = std::nullopt;
            if (!paren_stack.empty()) {
                top = paren_stack.back();
                paren_stack.pop_back();
            } else {
                last_was_operator = false;
                break;
            }
            expect(token::TokenType::left_paren, top);
            last_was_operator = false;
            last_ty = ty;
            continue;
        } else if (ty == token::TokenType::right_brace) {
            std::optional<token::Token> top = std::nullopt;
            if (!paren_stack.empty()) {
                top = paren_stack.back();
                paren_stack.pop_back();
            } else {
                last_was_operator = false;
                break;
            }
            expect(token::TokenType::left_brace, top);
            last_was_operator = false;
            last_ty = ty;
            continue;
        } else {
            if (paren_stack.empty())
                entirely_wrapped_in_parens = false;
            else
                entirely_wrapped_in_parens = entirely_wrapped_in_parens && paren_stack.front().type == token::TokenType::left_paren;
        }

        if (paren_stack.empty() && std::count(operators.begin(), operators.end(), ty)) {
            if (!last_was_operator) {
                ParseState new_ps = {
                    .info = ps->info,
                    .iter = TokenIter {
                        .tokens = operand_start,
                        .n_remain = ps->iter.tokens - operand_start
                    },
                    .errors = ps->errors,
                    .file = ps->file
                };
                parse_states.push_back(new_ps);
                epnis.push_back(EPNI {
                    .idx = parse_states.size() - 1,
                    .is_operator = false,
                    .loc = tok.loc,
                });
            }
            epnis.push_back(EPNI {
                .idx = static_cast<uint32_t>(ty),
                .is_operator = true,
                .loc = tok.loc,
            });
            last_was_operator = true;
            operand_start = ps->iter.tokens + 1;
        } else if (paren_stack.empty() && std::count(puncts.begin(), puncts.end(), ty)) {
            // encountered unexpected punctuation like `,` -> end of expression
            if (last_was_operator)
                // expect always throws
                expectNoneOf(puncts, tok);
            last_was_operator = false;
            break;
        } else {
            last_was_operator = false;
        }

        last_ty = ty;
    }

    // add the very last operand as well
    if (operand_start < ps->iter.tokens) {
        ParseState new_ps = {
            .info = ps->info,
            .iter = TokenIter {
                .tokens = operand_start,
                .n_remain = ps->iter.tokens - operand_start
            },
            .errors = ps->errors,
            .file = ps->file
        };
        parse_states.push_back(new_ps);
        epnis.push_back(EPNI {
            .idx = parse_states.size() - 1,
            .is_operator = false,
            .loc = operand_start->loc,
        });
    }

    if (!paren_stack.empty())
        unexpectedEof(ps->file);

    if (parse_states.empty()) {
        expectNoneOf(puncts, ps->peek());
        throw std::runtime_error("unreachable: line above should have thrown and been handled");
    }

    if (entirely_wrapped_in_parens) {
        // TODO handle tuples if I add them
        if (parse_states.size() != 1)
            throw std::runtime_error("unreachable: if expression is entirely wrapped in parens, it should be parsed as one thing and recursed into");
        auto &trunc_ps = parse_states.front();
        trunc_ps.iter.tokens++;
        trunc_ps.iter.n_remain -= 2;
        return parseExpression(&trunc_ps);
    } else if (parse_states.size() == 1) {
        // no operators, determine what parsing function to call (eg parseIfCond)
        ps = &parse_states.front();
        // first one must not be paren
        // TODO the problem is that, if multiple expressions are written after one another like f(x) {1} then the parser doesn't stop after the first one but tries to parse them all
        // -> find cases when it has to stop
        if (expectSome(ps->peek()).type == token::TokenType::left_paren)
            throw std::runtime_error("unreachable: it is not syntactically possible that a single-operand expression starts with a paren and is not fully wrapped in them; probably a bug with full wrap checking");

        auto tok = expectOneOf({
            token::TokenType::left_brace,
            token::TokenType::number,
            token::TokenType::ident,
            token::TokenType::if_kwd,
            token::TokenType::while_kwd
        }, ps->peek());
        auto loc = tok.loc;
        auto ty = tok.type;
        if (ty == token::TokenType::left_brace)
            return parseBlock(ps);
        else if (ty == token::TokenType::number) {
            if (!tok.meta)
                throw std::runtime_error("unreachable: number token was not assigned a meta");
            auto meta = tok.meta.value();
            std::unique_ptr<ast::Expr> expr = std::make_unique<ast::Constant>(loc, meta.number);
            return expr;
        } else if (ty == token::TokenType::ident) {
            auto tok2 = ps->peek(1);
            std::unique_ptr<ast::Expr> expr;
            if (!tok2 || tok2.value().type != token::TokenType::left_paren)
                expr = std::make_unique<ast::VarRef>(loc, std::move(std::string(tok.value.start, tok.value.length)));
            else
                expr = parseFunctionCall(ps);
            return expr;
        } else if (ty == token::TokenType::if_kwd) {
            return parseIfCond(ps);
        } else if (ty == token::TokenType::while_kwd) {
            return parseWhileLoop(ps);
        } else {
            throw std::runtime_error("unreachable: should have been checked for and should have thrown");
        }
    } else {
        // resolve all operands
        std::vector<std::unique_ptr<ast::Expr>> operands;
        for (auto &operand_ps : parse_states) {
            operands.push_back(parseExpression(&operand_ps));
        }
        std::vector<EPNI> new_epnis;
        for (auto const &ops : operators_by_prec) {
            bool prev_was_operator = true;
            for (uint32_t i = 0; i < epnis.size(); i++) {
                auto epni = epnis[i];
                if (epni.is_operator && std::count(ops.begin(), ops.end(), static_cast<token::TokenType>(epni.idx))) {
                    // fuse left and right operators (or just right if left is itself operator -> unary op)
                    if (prev_was_operator) {
                        if (i + 1 >= epnis.size())
                            throw std::runtime_error("unreachable: syntax error should have already been caught somewhere else");
                        auto next_epni = epnis[i + 1];
                        if (next_epni.is_operator) {
                            auto fake_token = token::Token {
                                .value = StringRef {
                                    .start = 0,
                                    .length = 0
                                },
                                .type = token::TokenType::invalid,
                                .meta = std::nullopt,
                                .loc = epni.loc,
                            };
                            throw UnexpectedTokenError("expected an operand to the right of an operator, but got another operator.", fake_token);
                        }
                        auto rhs = std::move(operands[next_epni.idx]);
                        std::unique_ptr<ast::Expr> unary_op = std::make_unique<ast::UnaryOp>(
                            epni.loc,
                            std::move(rhs),
                            ast::unaryOpTypeFromTokenType(static_cast<token::TokenType>(epni.idx))
                        );
                        operands.push_back(std::move(unary_op));
                        new_epnis.push_back(EPNI {.idx = operands.size() - 1, .is_operator = false});
                    } else {
                        if (i + 1 >= epnis.size() || i == 0)
                            throw std::runtime_error("unreachable: syntax error should have already been caught somewhere else");
                        auto prev_epni = new_epnis.back();
                        auto next_epni = epnis[i + 1];
                        if (next_epni.is_operator) {
                            auto fake_token = token::Token {
                                .value = StringRef {
                                    .start = 0,
                                    .length = 0
                                },
                                .type = token::TokenType::invalid,
                                .meta = std::nullopt,
                                .loc = epni.loc,
                            };
                            throw UnexpectedTokenError("expected an operand to the right of an operator, but got another operator.", fake_token);
                        }
                        if (prev_epni.is_operator)
                            throw std::runtime_error("unreachable: should be interpreted as unary not binary op");

                        auto lhs = std::move(operands[prev_epni.idx]);
                        auto rhs = std::move(operands[next_epni.idx]);
                        std::unique_ptr<ast::Expr> binary_op = std::make_unique<ast::BinaryOp>(
                            epni.loc,
                            std::move(lhs),
                            std::move(rhs),
                            ast::binaryOpTypeFromTokenType(static_cast<token::TokenType>(epni.idx))
                        );
                        operands.push_back(std::move(binary_op));
                        new_epnis[new_epnis.size() - 1] = EPNI {.idx = operands.size() - 1, .is_operator = false};
                    }
                    i++;
                } else {
                    new_epnis.push_back(epni);
                }
                prev_was_operator = epni.is_operator && !std::count(ops.begin(), ops.end(), static_cast<token::TokenType>(epni.idx));
            }
            epnis = std::move(new_epnis);
            new_epnis = std::vector<EPNI>();
        }

        if (epnis.size() != 1)
            throw std::runtime_error("unreachable: operation fusion bug, fused into " + std::to_string(epnis.size()) + " operations instead of 1");
        auto epni = epnis[0];
        if (epni.is_operator)
            throw std::runtime_error("unreachable: operation fusion bug, result EPNI is marked as operator");
        auto result = std::move(operands[epni.idx]);
        return result;
    }
}

std::unique_ptr<ast::If> parseIfCond(ParseState *ps) {
    auto loc = expect(token::TokenType::if_kwd, ps->next()).loc;
    expectNot(token::TokenType::left_brace, ps->peek());
    auto limited_ps = splitIterAtTTInplace(token::TokenType::left_brace, ps);
    auto cond = parseExpression(&limited_ps);
    auto branch = parseBlock(ps);
    std::optional<std::unique_ptr<ast::Expr>> else_branch = std::nullopt;
    if (ps->peek() && ps->peek().value().type == token::TokenType::else_kwd) {
        ps->next();
        expectOneOf({token::TokenType::left_brace, token::TokenType::if_kwd}, ps->peek());
        else_branch = parseExpression(ps);
    }
    auto if_cond = std::make_unique<ast::If>(loc, std::move(cond), std::move(branch), std::move(else_branch));
    return if_cond;
}

std::unique_ptr<ast::While> parseWhileLoop(ParseState *ps) {
    auto loc = expect(token::TokenType::while_kwd, ps->next()).loc;
    auto limited_ps = splitIterAtTTInplace(token::TokenType::left_brace, ps);
    auto cond = parseExpression(&limited_ps);
    auto branch = parseBlock(ps, false, false);
    auto while_loop = std::make_unique<ast::While>(loc, std::move(cond), std::move(branch));
    return while_loop;
}

std::unique_ptr<ast::FunctionCall> parseFunctionCall(ParseState *ps) {
    auto ident_tok = expect(token::TokenType::ident, ps->next());
    auto name = std::string(ident_tok.value.start, ident_tok.value.length);
    expect(token::TokenType::left_paren, ps->next());
    std::vector<std::unique_ptr<ast::Expr>> args;
    bool last_was_comma = true;
    while (true) {
        if (last_was_comma) {
            auto expr = parseExpression(ps);
            args.push_back(std::move(expr));
            last_was_comma = false;
        } else {
            auto next_tok = expectOneOf({token::TokenType::comma, token::TokenType::right_paren}, ps->next());
            if (next_tok.type == token::TokenType::right_paren) break;
            last_was_comma = true;
        }
    }
    auto fc = std::make_unique<ast::FunctionCall>(ident_tok.loc, std::move(name), std::move(args));
    return fc;
}

std::unique_ptr<ast::DeclAssignment> parseDeclAssignment(ParseState *ps) {
    auto loc = expect(token::TokenType::let_kwd, ps->next()).loc;
    auto ident_tok = expect(token::TokenType::ident, ps->next());
    auto name = std::string(ident_tok.value.start, ident_tok.value.length);
    auto equals_or_semi = expectOneOf({token::TokenType::equals, token::TokenType::semicolon}, ps->next());
    std::optional<std::unique_ptr<ast::Expr>> value = std::nullopt;
    if (equals_or_semi.type == token::TokenType::equals) {
        value = parseExpression(ps);
        expect(token::TokenType::semicolon, ps->next());
    }
    auto stmt = std::make_unique<ast::DeclAssignment>(loc, std::move(name), std::move(value));
    return stmt;
}

std::unique_ptr<ast::Return> parseReturn(ParseState *ps) {
    auto loc = expect(token::TokenType::return_kwd, ps->next()).loc;
    auto expr = parseExpression(ps);
    expect(token::TokenType::semicolon, ps->next());
    auto stmt = std::make_unique<ast::Return>(loc, std::move(expr));
    return stmt;
}

std::unique_ptr<ast::FunctionDef> parseFunctionDef(ParseState *ps) {
    auto loc = expect(token::TokenType::fn_kwd, ps->next()).loc;

    auto ident_tok = expect(token::TokenType::ident, ps->next());
    auto name = std::string(ident_tok.value.start, ident_tok.value.length);

    expect(token::TokenType::left_paren, ps->next());
    std::vector<std::string> args;
    bool last_was_comma = true;
    while (true) {
        if (last_was_comma) {
            auto arg = expect(token::TokenType::ident, ps->next());
            args.push_back(std::string(arg.value.start, arg.value.length));
            last_was_comma = false;
        } else {
            auto next_tok = expectOneOf({token::TokenType::comma, token::TokenType::right_paren}, ps->next());
            if (next_tok.type == token::TokenType::right_paren) break;
            last_was_comma = true;
        }
    }
    
    expect(token::TokenType::left_brace, ps->peek());
    std::unique_ptr<ast::Block> block = parseBlock(ps);
    auto stmt = std::make_unique<ast::FunctionDef>(loc, std::move(name), std::move(args), std::move(block));
    return stmt;
}

std::unique_ptr<ast::Statement> parseStatement(ParseState *ps, bool is_toplevel) {
    auto keyword_tok = expectSome(ps->peek());
    auto ty = keyword_tok.type;
    if (ty == token::TokenType::let_kwd) return parseDeclAssignment(ps);
    if (ty == token::TokenType::fn_kwd) {
        if (is_toplevel)
            return parseFunctionDef(ps);
        else
            expectNot(token::TokenType::fn_kwd, keyword_tok);
    }
    if (ty == token::TokenType::return_kwd) {
        if (is_toplevel)
            expectNot(token::TokenType::return_kwd, keyword_tok);
        else
            return parseReturn(ps);
    }

    // expression as a statement (eg `function(xyz);`)
    auto expr = parseExpression(ps);
    // those statements with an attached block don't need semicolons

    std::unique_ptr<ast::Statement> stmt;
    if (expectSome(ps->peek()).type == token::TokenType::equals) {
        // parse an expr assignment: `x = 5; *((char *)y[0]) = 6;` and so on (replaces parseAssignment function for efficiency)
        ps->next();
        auto value = parseExpression(ps);
        expect(token::TokenType::semicolon, ps->next());
        stmt = std::make_unique<ast::Assignment>(keyword_tok.loc, std::move(expr), std::move(value));
    } else {
        if (ty != token::TokenType::if_kwd
            && ty != token::TokenType::while_kwd
            && ty != token::TokenType::left_brace)
            expect(token::TokenType::semicolon, ps->next());
        stmt = std::make_unique<ast::ExprStmt>(keyword_tok.loc, std::move(expr));
    }
    return stmt;
}

std::unique_ptr<ast::Block> parseBlock(ParseState *ps, bool is_toplevel/* = false*/, bool allow_implicit_return/* = true*/) {
    // NOTE: this is safe when parsing toplevel because of eof token
    auto loc = expectSome(ps->peek()).loc;
    if (!is_toplevel)
        expect(token::TokenType::left_brace, ps->next());

    uint32_t n_parens = 0;
    uint32_t block_end = -1;
    uint32_t n_statements = 0;
    uint32_t init_remain = ps->iter.n_remain;
    bool broke_out = false;
    // TODO this is gonna be really hard: figure out how to compute num statements
    // where not every statement ends on semicolon or has a fn kwd. anything that has a block
    // attached to it can also be a statement without a semicolon. however, it could also be a return if it is the last thing in the block.
    // glhf future me
    for (uint32_t i = 0; i < ps->iter.n_remain - 1; i++) {
        auto ty = ps->iter.tokens[i].type;
        if (ty == token::TokenType::left_brace)
            n_parens++;
        else if (ty == token::TokenType::right_brace) {
            if (n_parens)
                n_parens--;
            else {
                block_end = i;
                broke_out = true;
                break;
            }
        }
        if (!n_parens && (
            ty == token::TokenType::semicolon
            || ty == token::TokenType::fn_kwd
        ))
            n_statements++;
    }
    if (!n_parens && !broke_out)
        block_end = ps->iter.n_remain - 1;  // eof token
    
    std::vector<std::unique_ptr<ast::Statement>> statements;
    for (; n_statements; n_statements--) {
        auto current_tok_ = ps->peek();
        if (!current_tok_) {
            auto err = Error {
                .line = 0,
                .file = ps->file,
                .msg = std::string("unexpected end of file")
            };
            ps->errors->push_back(std::move(err));
            break;
        }
        auto current_tok = current_tok_.value();
        try {
            auto stmt = parseStatement(ps);
            statements.push_back(std::move(stmt));
        } catch (UnexpectedTokenError e) {
            auto err = Error {
                .line = current_tok.loc.line,
                .file = current_tok.loc.file,
                .msg = e.getMessage()
            };
            ps->errors->push_back(std::move(err));
        }
        // consume redundant semicolons (that were interpreted as statements -> decrease n_statements)
        for (; ps->peek() && ps->peek().value().type == token::TokenType::semicolon; n_statements--)
            ps->next();
    }

    auto current_tok_ = ps->peek();
    std::optional<std::unique_ptr<ast::Expr>> result = std::nullopt;
    if (allow_implicit_return && current_tok_) {
        auto current_tok = current_tok_.value();
        if (init_remain - ps->iter.n_remain < block_end) {
            try {
                result = parseExpression(ps);
            } catch (UnexpectedTokenError e) {
                auto err = Error {
                    .line = current_tok.loc.line,
                    .file = current_tok.loc.file,
                    .msg = e.getMessage()
                };
                ps->errors->push_back(std::move(err));
            }
        }
    }

    if (!is_toplevel)
        expect(token::TokenType::right_brace, ps->next());
    // TODO can invalid code lead to the below being wrong?
    // else
    //     expect(token::TokenType::eof, ps->next());

    auto block = std::make_unique<ast::Block>(loc, std::move(statements), std::move(result), is_toplevel);
    return block;
}

std::unique_ptr<ast::Block> parse(StringRef file, std::vector<token::Token> const &tokens, std::vector<Error> *errors) {
    ParseState ps = {
        .info = TokenInfo {.start = tokens.data(), .length = tokens.size()},
        .iter = TokenIter {.tokens = tokens.data(), .n_remain = tokens.size()},
        .errors = errors,
        .file = file
    };
    return parseBlock(&ps, true);
}
}  // namespace parser
