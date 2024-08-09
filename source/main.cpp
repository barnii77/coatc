#include <iostream>
#include <string>
#include <cstdint>
#include <fstream>
#include <filesystem>

#include "lib.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "llvm_codegen.hpp"

#define RUN_ONLY_TMPTEST
#define ERR_N_SURROUND_LINES_PRINTED 3
#define ARROW_TYPE 3

// todo add a way to also load an ast from json
// TODO break/continue statements
// TODO add comparison operators
// TODO ~~switch from double to byte and~~ make it compile to mattbatwings redstone cpu? (using custom llvm backend! and my own vm and lowering later? maybe also mlir??)
// todo write preprocessor
// todo write an always-returns analysis pass that determines if a void block return type is fine because there is a return statement that is guaranteed to be executed (kinda like the ! type in rust)

enum class ErrorKind {
    parser_error,
    codegen_error,
    codegen_warning
};

struct UniversalError {
    ErrorKind kind;
    LocationInfo loc;
    std::string msg;

    UniversalError(parser::Error err) : kind(ErrorKind::parser_error), loc(err.loc), msg(std::move(err.msg)) {};
    UniversalError(codegen::Error err) : kind(ErrorKind::codegen_error), loc(err.loc), msg(std::move(err.msg)) {};
    UniversalError(codegen::Warning warn) : kind(ErrorKind::codegen_warning), loc(warn.loc), msg(std::move(warn.msg)) {};
};

void printErrMsg(UniversalError const &err) {
    std::string errty = "Error";
    if (err.kind == ErrorKind::parser_error)
        errty = "Syntax Error";
    else if (err.kind == ErrorKind::codegen_error)
        errty = "Codegen Error";
    else if (err.kind == ErrorKind::codegen_warning)
        errty = "Codegen Warning";
    std::cout << errty << " encountered: " << err.msg;
}

void printErrLoc(UniversalError const &err) {
    std::cout
        << "In file " << err.loc.file.start
        << ", line " << (err.loc.line == 0 ? std::string("<end of file>") : std::to_string(err.loc.line))
        << ", column " << std::to_string(err.loc.column)
        << ":";
}

void prettyPrintErr(UniversalError const &err, std::vector<StringRef> lines) {
    uint32_t line = err.loc.line;
    uint32_t column = err.loc.column;
    auto line_ = lines.at(line);
    uint32_t line_num_len = std::to_string(line).length();
    column += line_num_len + 2;
    printErrLoc(err);
    auto n = ERR_N_SURROUND_LINES_PRINTED;
    char const *color_esc_seq = err.kind == ErrorKind::codegen_warning ? "\x1b[33m" : "\x1b[91m";
    std::cout << std::endl;
    for (int32_t li = static_cast<int32_t>(line) - n; li <= static_cast<int32_t>(line) + n; li++) {
        if (li > 0 && li <= lines.size()) {
            auto li_str = std::to_string(li);
            std::cout << li_str;
            for (uint32_t j = li_str.length(); j < line_num_len; j++)
                std::cout << ' ';
            auto curline = lines.at(li - 1);
            std::cout  << "| " << std::string(curline.start, curline.length) << std::endl;
        }
        if (li == line) {
            for (uint32_t i = 0; i < column; i++)
                std::cout << ' ';
            if (ARROW_TYPE >= 2)
                std::cout << color_esc_seq << "\x1b[4mΛ\x1b[24m" << std::endl;
            else
                std::cout << "A" << std::endl;
            if (ARROW_TYPE >= 3) {
                for (uint32_t i = 0; i < column - 1; i++)
                    std::cout << ' ';
                std::cout << "\x1b[4m/|\\\x1b[24m" << std::endl;
            } else if (ARROW_TYPE == 1) {
                for (uint32_t i = 0; i < column - 1; i++)
                    std::cout << ' ';
                std::cout << "/|\\" << std::endl;
            }
            for (uint32_t i = 0; i < column; i++)
                std::cout << ' ';
            if (ARROW_TYPE >= 2)
                std::cout << "|\x1b[4m      ";
            else
                std::cout << "|______";
            printErrMsg(err);
            std::cout << "\x1b[24m\x1b[0m" << std::endl << std::endl;
        }
    }
}

#define PRINT_ERROR(err_expr) { \
    UniversalError const err(err_expr); \
    prettyPrintErr(err, code_lines); \
}

void printErrorsAndWarnings(
    std::vector<StringRef> const &code_lines,
    SizedArray<parser::Error> pr_errors,
    SizedArray<codegen::Error> cg_errors,
    SizedArray<codegen::Warning> cg_warnings
) {
    while (pr_errors.size() || cg_errors.size() || cg_warnings.size()) {
        uint32_t prel = pr_errors.size() ? pr_errors.front().loc.line : -1;
        uint32_t cgel = cg_errors.size() ? cg_errors.front().loc.line : -1;
        uint32_t cgwl = cg_warnings.size() ? cg_warnings.front().loc.line : -1;
        if (prel <= cgel && prel <= cgwl) {
            PRINT_ERROR(pr_errors.consumeFront());
        } else if (cgel <= prel && cgel <= cgwl) {
            PRINT_ERROR(cg_errors.consumeFront());
        } else {
            PRINT_ERROR(cg_warnings.consumeFront());
        }
        std::cout << std::endl;
    }
}

void dbgPrintTokens(std::vector<token::Token> const &tokens) {
    for (auto const &token : tokens) {
        std::cout << "Token: " << (uint64_t) token.value.start << " " << token.value.length << " " << static_cast<uint32_t>(token.type) << std::endl;
        std::cout << "Token value: ";
        if (token.value.length == 0) std::cout << "<eof>";
        for (int i = 0; i < token.value.length; i++) {
            std::cout << token.value.start[i];
        }
        std::cout << std::endl;
    }
}

void dbgPrintIR(std::string const &ir) {
    std::cout << ir << std::endl << std::endl;
}

void dbgPrintAST(ast::Block *block) {
    std::cout << block->toJsonString() << std::endl;
}

std::string compileToIR(StringRef file, StringRef code) {
    auto const tokens = token::tokenize(file, code);
    // dbgPrintTokens(tokens);

    std::vector<StringRef> code_lines;
    uint32_t start_offset = 0;
    for (uint32_t i = 0; i < code.length; i++) {
        if (code.start[i] == '\n') {
            code_lines.push_back(StringRef {.start = code.start + start_offset, .length = i - start_offset});
            start_offset = i + 1;
        }
    }
    if (start_offset < code.length - 1)
        code_lines.push_back(StringRef {.start = code.start + start_offset, .length = code.length - 1 - start_offset});

    std::vector<parser::Error> pr_errors;
    auto block = parser::parse(file, tokens, &pr_errors);
    // dbgPrintAST(block.get());
    std::vector<codegen::Error> cg_errs;
    std::vector<codegen::Warning> cg_warns;
    codegen::State state;
    codegen::Context ctx = codegen::newContext(file, &cg_errs, &cg_warns, &state);
    block->codegen(&ctx);
    std::string ir = codegen::dumpIR(&ctx);
    // dbgPrintIR(ir);
    printErrorsAndWarnings(code_lines, pr_errors, cg_errs, cg_warns);

    return ir;
}

int main(int argc, char **argv) {
#ifdef RUN_ONLY_TMPTEST
    argc = 1;
#endif

    if (argc == 1) {
        for (auto const &e : std::filesystem::directory_iterator("test/code_samples")) {
            std::string path = e.path();
#ifdef RUN_ONLY_TMPTEST
            if (path != "test/code_samples/tmptest.bpl")
                continue;
#else
            if (path == "test/code_samples/tmptest.bpl")
                continue;
#endif
            std::cout << "RUNNNIG TEST " << path << std::endl << std::endl;
            std::ifstream in_file(path);
            std::stringstream content_strm;
            content_strm << in_file.rdbuf();
            std::string content(content_strm.str());
            auto file_sr = StringRef {
                .start = path.c_str(),
                .length = path.length(),
            };
            std::string ir = compileToIR(file_sr, StringRef {.start = content.c_str(), .length = content.length()});
            std::string out_path = std::string("test/llvmir_out") + &path.c_str()[path.find_last_of('/')] + ".ll";
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
            std::string ir = compileToIR(file_sr, StringRef {.start = content.c_str(), .length = content.length()});
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
