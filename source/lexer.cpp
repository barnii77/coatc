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
        .column = column, \
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
        .column = column, \
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

std::vector<Token> tokenize(StringRef file, StringRef code_) {
    char const *code = code_.start;
    uint32_t length = code_.length;
    std::vector<Token> result;
    uint32_t line = 1;
    uint32_t column = 0;
    bool is_comment = false;
    bool last_was_slash = false;
    bool last_was_zero = false;
    bool hex_mode = false;
    for (uint32_t i = 0; i < length; i++, column++) {
        char c = code[i];
        if (last_was_slash && c == '/') {
            is_comment = true;
            result.pop_back();  // remove the slash that has already been added
        }

        if (c == ' ')
            continue;
        else if (c == '\n') {
            line++;
            column = -1;
            is_comment = false;
            continue;
        }

        if (is_comment)
            continue;

        if (last_was_zero && c == 'x') {
            last_was_zero = false;
            hex_mode = true;
            continue;
        } else if (last_was_zero) {
            result.push_back(
                Token {
                    .value = StringRef {
                        .start = code + i - 1,
                        .length = 1,
                    },
                    .type = TokenType::number,
                    .meta = TokenMeta {.number = 0},
                    .loc = LocationInfo {
                        .line = line,
                        .column = column,
                        .file = file,
                    }
                }
            );
        }

        if (c == '0') {
            last_was_zero = true;
            continue;
        } else
            last_was_zero = false;


        last_was_slash = c == '/';

        uint32_t start = i;
        if ((std::isalpha(c) || c == '_') && !hex_mode) {
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
                            .column = column,
                            .file = file,
                        },
                    }
                );
            }
        } else if (std::isdigit(c) || (hex_mode && (('a' <= c && c <= 'f') || ('A' <= c && c <= 'F')))) {
            uint64_t num = 0;
            uint64_t base = hex_mode ? 16 : 10;
            while (i < length && (std::isdigit(c) || (hex_mode && (('a' <= c && c <= 'f') || ('A' <= c && c <= 'F'))))) {
                if (hex_mode && 'A' <= c && c <= 'F')
                    c -= 'A' - 10;
                else if (hex_mode && 'a' <= c && c <= 'f')
                    c -= 'a' - 10;
                else
                    c -= '0';
                num = base * num + c;
                if (++i < length) c = code[i];
            }
            i--;
            result.push_back(
                Token {
                    .value = StringRef {
                        .start = code + start - hex_mode * 2,
                        .length = i - start + 1 + hex_mode * 2,
                    },
                    .type = TokenType::number,
                    .meta = TokenMeta {.number = num},
                    .loc = LocationInfo {
                        .line = line,
                        .column = column,
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
        hex_mode = false;
    }
    if (last_was_zero)
        result.push_back(
            Token {
                .value = StringRef {
                    .start = code + length - 1,
                    .length = 1,
                },
                .type = TokenType::number,
                .meta = TokenMeta {.number = 0},
                .loc = LocationInfo {
                    .line = line,
                    .column = column,
                    .file = file,
                }
            }
        );
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
                .column = column,
                .file = file,
            }
        }
    );
    return result;
}
}  // namespace token
