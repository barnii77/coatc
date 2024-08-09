#include <iostream>
#include <string>
#include <cstdint>
#include <fstream>
#include <filesystem>

#include "lib.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "llvm_codegen.hpp"

// TODO add a way to also load an ast from json
// TODO break/continue statements
// TODO add comparison operators
// TODO ~~switch from double to byte and~~ make it compile to mattbatwings redstone cpu? (using custom llvm backend! and my own vm and lowering later? maybe also mlir??)

// TODO better compiler errors by letting one pass custom messages with expect/expectNot/...
// TODO write preprocessor

// TODO fix unary operator parsing
// TODO write an always-returns analysis pass that determines if a void block return type is fine because there is a return statement that is guaranteed to be executed (kinda like the ! type in rust)

std::string compile_to_ir(StringRef file, std::string code) {
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

    std::cout << "Now the codegen!" << std::endl << std::endl << std::endl;
    std::vector<codegen::Error> cg_errs;
    codegen::State state;
    codegen::Context ctx = codegen::newContext(file, &cg_errs, &state);
    block->codegen(&ctx);
    std::string ir = codegen::dumpIR(&ctx);
    std::cout << ir << std::endl;
    std::cout << "codegen errors:" << std::endl;
    for (auto const &err : cg_errs) {
        std::cout << "Error encountered in file " << err.loc.file.start << " on line " << std::to_string(err.loc.line) << ": " << err.msg << std::endl;
    }
    return ir;
}

int main(uint32_t argc, char **argv) {
    if (argc == 1) {
        for (auto const &e : std::filesystem::directory_iterator("test/code_samples")) {
            std::string path = e.path();
            std::ifstream in_file(path);
            std::stringstream content_strm;
            content_strm << in_file.rdbuf();
            std::string content(content_strm.str());
            auto file_sr = StringRef {
                .start = path.c_str(),
                .length = path.length(),
            };
            std::string ir = compile_to_ir(file_sr, content);
            std::string out_path = std::string("test/llir_out") + &path.c_str()[path.find_last_of('/')] + ".ll";
            std::cout << "writing ir to " << out_path << std::endl;
            std::ofstream out_file(out_path);
            out_file << ir;
        }
    } else {
        for (uint32_t i = 1; i < argc; i++) {
            std::string path(argv[i]);
            std::ifstream in_file(path);
            std::stringstream content_strm;
            content_strm << in_file.rdbuf();
            std::string content(content_strm.str());
            auto file_sr = StringRef {
                .start = path.c_str(),
                .length = path.length(),
            };
            std::string ir = compile_to_ir(file_sr, content);
            std::string out_path = path + ".ll";
            std::ofstream out_file(out_path);
            out_file << ir;
        }
    }
    // auto const code = std::string {
    //     "fn main(b) {\n"
    //         "let a = 3;\n"
    //         "b = add1(a * 3 - 2);\n"
    //         "if (1 - a) {\n"
    //             "return 1;\n"
    //             "0\n"
    //         "} else {\n"
    //             "a\n"
    //         "};\n"
    //         "print(a);\n"
    //         "return 0;\n"
    //     "}"

    //     "fn add1(x) {\n"
    //         "return x + 1;\n"
    //     "}\n"
    // };
    return 0;
}
