#include "lexer.hpp"

#define KWD_TOKEN(kind) \
Token { \
    .value = StringRef { \
        .start = code + start, \
        .length = i - start + 1, \
    }, \
    .type = TokenType::kind, \
    .meta = std::nullopt, \
    .loc = LocationInfo { \
        .line = line, \
        .file = file, \
    }, \
}

#define SWITCH_CHARS_BRANCH(kind) \
result.push_back(Token { \
    .value = StringRef { \
        .start = code + start, \
        .length = i - start + 1, \
    }, \
    .type = TokenType::kind, \
    .meta = std::nullopt, \
    .loc = LocationInfo { \
        .line = line, \
        .file = file, \
    }, \
}); break;

namespace token {
std::string displayTokenType(TokenType t) {
    if (t == TokenType::eof) return "eof";
    if (t == TokenType::left_paren) return "(";
    if (t == TokenType::right_paren) return ")";
    if (t == TokenType::left_brace) return "{";
    if (t == TokenType::right_brace) return "}";
    if (t == TokenType::comma) return ",";
    if (t == TokenType::plus) return "+";
    if (t == TokenType::minus) return "-";
    if (t == TokenType::asterisk) return "*";
    if (t == TokenType::slash) return "/";
    if (t == TokenType::percent) return "%";
    if (t == TokenType::semicolon) return ";";
    if (t == TokenType::equals) return "=";
    if (t == TokenType::fn_kwd) return "fn";
    if (t == TokenType::let_kwd) return "let";
    if (t == TokenType::if_kwd) return "if";
    if (t == TokenType::else_kwd) return "else";
    if (t == TokenType::while_kwd) return "while";
    if (t == TokenType::return_kwd) return "return";
    if (t == TokenType::ident) return "ident";
    if (t == TokenType::number) return "number";
    if (t == TokenType::invalid) return "invalid";
    return "<UnknownTokenType>";
}

std::vector<Token> tokenize(StringRef file, char const *code, uint32_t length) {
    std::vector<Token> result;
    uint32_t line = 1;
    bool is_comment = false;
    bool last_was_slash = false;
    for (uint32_t i = 0; i < length; i++) {
        char c = code[i];
        if (last_was_slash && c == '/')
            is_comment = true;

        if (c == ' ')
            continue;
        else if (c == '\n') {
            line++;
            is_comment = false;
        }

        last_was_slash = c == '/';
        if (is_comment)
            continue;

        uint32_t start = i;
        if (std::isalpha(c) || c == '_') {
            std::string ident;
            while (i < length && (std::isalpha(c) || c == '_' || std::isdigit(c))) {
                ident += c;
                if (++i < length) c = code[i];
            }
            i--;
            if (ident == "fn") {
                result.push_back(
                    KWD_TOKEN(fn_kwd)
                );
            } else if (ident == "if") {
                result.push_back(
                    KWD_TOKEN(if_kwd)
                );
            } else if (ident == "else") {
                result.push_back(
                    KWD_TOKEN(else_kwd)
                );
            } else if (ident == "while") {
                result.push_back(
                    KWD_TOKEN(while_kwd)
                );
            } else if (ident == "return") {
                result.push_back(
                    KWD_TOKEN(return_kwd)
                );
            } else if (ident == "let") {
                result.push_back(
                    KWD_TOKEN(let_kwd)
                );
            } else {
                StringRef ident_str = {
                    .start = code + start,
                    .length = i - start + 1,
                };
                result.push_back(
                    Token {
                        .value = StringRef {
                            .start = code + start,
                            .length = i - start + 1,
                        },
                        .type = TokenType::ident,
                        .meta = std::nullopt,
                        .loc = LocationInfo {
                            .line = line,
                            .file = file,
                        },
                    }
                );
            }
        } else if (std::isdigit(c) || c == '.') {
            std::string num_buffer;
            while (i < length && (std::isdigit(c) || c == '.')) {
                num_buffer += c;
                if (++i < length) c = code[i];
            }
            i--;
            uint8_t num = std::stoi(num_buffer);
            result.push_back(
                Token {
                    .value = StringRef {
                        .start = code + start,
                        .length = i - start + 1,
                    },
                    .type = TokenType::number,
                    .meta = TokenMeta {.number = num},
                    .loc = LocationInfo {
                        .line = line,
                        .file = file,
                    }
                }
            );
        } else switch (c) {
            case '+': SWITCH_CHARS_BRANCH(plus);
            case '-': SWITCH_CHARS_BRANCH(minus);
            case '*': SWITCH_CHARS_BRANCH(asterisk);
            case '/': SWITCH_CHARS_BRANCH(slash);
            case '%': SWITCH_CHARS_BRANCH(percent);
            case ';': SWITCH_CHARS_BRANCH(semicolon);
            case ',': SWITCH_CHARS_BRANCH(comma);
            case '=': SWITCH_CHARS_BRANCH(equals);
            case '(': SWITCH_CHARS_BRANCH(left_paren);
            case ')': SWITCH_CHARS_BRANCH(right_paren);
            case '{': SWITCH_CHARS_BRANCH(left_brace);
            case '}': SWITCH_CHARS_BRANCH(right_brace);
            default: break;
        }
    }
    result.push_back(
        Token {
            .value = StringRef {
                .start = code,
                .length = 0,
            },
            .type = TokenType::eof,
            .meta = std::nullopt,
            .loc = LocationInfo {
                .line = line,
                .file = file,
            }
        }
    );
    return result;
}
}  // namespace token
