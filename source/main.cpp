#include <iostream>
#include <string>
#include <cstdint>

#include "lib.hpp"
#include "lexer.hpp"
#include "parser.hpp"

int main() {
    // TODO add a way to also load an ast from json
    // TODO break/continue statements
    // TODO add comparison operators
    // TODO ~~switch from double to byte and~~ make it compile to mattbatwings redstone cpu? (using custom llvm backend! and my own vm and lowering later? maybe also mlir??)
    // TODO fix the statement counting (probably by removing it and finding a better way?)
    
    // TODO better compiler errors by letting one pass custom messages with expect/expectNot/...
    // TODO write preprocessor
    // TODO jit generate forward declarations before codegen
    
    auto const code = std::string {
        // "let x = 1.0 + 2;\n"
        // "let a = {print(2); 15;};" // else if (y) {5;};\n"
        // "fn abc(xyz) {return 0;}"// else {5;};"
        // "print(x);\n"
        // "while (x) {y;}; print(x);"
        // "x = 3;\n-y = 2;\n"
        // "if;"  // invalid example for testing compiler errors
        // "if (x) {\nif (y) {z}};"
        // "let i = 3 + 2 * 3 + 4 % 3;"
    };
    std::string filename = "test.bpl";
    StringRef file = {
        .start = filename.c_str(),
        .length = filename.length()
    };
    auto const tokens = token::tokenize(file, code.c_str(), code.length());
    if (false) {
        for (auto const &token : tokens) {
            std::cout << "Token: " << (uint64_t) token.value.start << " " << token.value.length << " " << static_cast<uint32_t>(token.type) << std::endl;
            std::cout << "Token value: ";
            if (token.value.length == 0) std::cout << "<eof>";
            for (int i = 0; i < token.value.length; i++) {
                std::cout << token.value.start[i];
            }
            std::cout << std::endl;
        }
        std::cout << "Now the parser" << std::endl;
    }
    std::vector<parser::Error> errors;
    auto block = parser::parse(file, tokens, &errors);
    for (auto err : errors) {
        std::cout << "Error encountered on line "
            << (err.line == 0 ? std::string("<end of file>") : std::to_string(err.line))
            << " in file " << err.file.start << ": " << err.msg << std::endl;
    }
    if (errors.empty())
        std::cout << "No parser errors" << std::endl;
    std::cout << block->toJsonString() << std::endl;
    return 0;
}
