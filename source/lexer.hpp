#pragma once

#include "lib.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace token {
typedef enum class TokenType : uint32_t {
    eof,

    left_paren,
    right_paren,
    left_brace,
    right_brace,

    comma,
    plus,
    minus,
    asterisk,
    slash,
    percent,
    semicolon,
    equals,

    fn_kwd,
    let_kwd,
    if_kwd,
    else_kwd,
    while_kwd,
    return_kwd,

    ident,
    number,
    invalid,
} TokenType;

std::string displayTokenType(TokenType t);

typedef union TokenMeta {
    uint64_t number;
} TokenMeta;

typedef struct Token {
    StringRef value;
    TokenType type;
    std::optional<TokenMeta> meta;
    LocationInfo loc;
} Token;

std::vector<Token> tokenize(StringRef file, StringRef code_);
}  // namespace Token
