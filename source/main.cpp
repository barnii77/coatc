#include "llvm/Target/TargetMachine.h"
#include "llvm/IR/Module.h"
#include <iostream>
#include <string>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <functional>

#include "lib.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "LLVMCodeGen/codegen.hpp"
#include "LLVMCodeGen/optimization.hpp"
#include "LLVMCodeGen/lowering.hpp"
#include "LLVMCodeGen/external_linking.hpp"
#include "message_strings.hpp"

#define ERR_N_SURROUND_LINES_PRINTED 3
#define ARROW_TYPE 3
#define RUN_TESTS_ON_EMPTY_PARAMS

enum class CompilerOutKind {
    tokens,
    ast,
    llvm_ir,
    optimized_llvm_ir,
    asm_,
    bitcode,
    obj,
    combined_obj,
    exe
};

// TODO I feel like when llvm.lifetime.start is being called in the ir the size parameter should be passed as bytes but seems to be bits instead
// todo add a way to also load an ast from json
// TODO break/continue statements: when adding declaration into declarations block, also optionally add lifetime ends to a `break block` and a `continue block`. these are inserted by loop codegen into state. state contains nullable pointers to these.
// TODO add comparison operators
// todo write preprocessor
// todo write an always-returns analysis pass that determines if a void block return type is fine because there is a return statement that is guaranteed to be executed (kinda like the ! type in rust)
// TODO fix extern and externc keywords (generate invalid code for some reason)
// TODO add extern keyword for global vars
// TODO allow regular LTO using same function that does ThinLTO atm (FIXME that function doesnt work either)
// TODO make the optimization pipeline take an "lto_later" parameter that controls if it should run per module def pipeline or thin/regular lto pipeline

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

void printTokens(std::stringstream &out, std::vector<token::Token> const &tokens) {
    for (auto const &token : tokens) {
        out << "Token: " << (uint64_t) token.value.start << " " << token.value.length << " " << static_cast<uint32_t>(token.type) << std::endl;
        out << "Token value: ";
        if (token.value.length == 0) out << "<eof>";
        for (int i = 0; i < token.value.length; i++) {
            out << token.value.start[i];
        }
        out << std::endl;
    }
}

void printIR(std::stringstream &out, std::string const &ir) {
    out << ir << std::endl << std::endl;
}

void printAST(std::stringstream &out, ast::Block *block) {
    out << block->toJsonString() << std::endl;
}

typedef struct SourceFileInfo {
    StringRef file;
    StringRef code;
} SourceFileInfo;

typedef struct OutFileInfo {
    std::string file;
    std::string content;
} OutFileInfo;

#define COMPILE_ALL_FILE_DONE() \
outs.push_back(OutFileInfo {.file = std::string(file_info.file.start, file_info.file.length), .content = out.str()}); \
continue;

std::vector<OutFileInfo> compileAll(
    std::vector<SourceFileInfo> file_infos,
    CompilerOutKind out_kind,
    OptLevel opt_level,
    LTOKind lto_kind,
    std::string out_filename,
    std::vector<std::string> const &link_static_libs,
    std::vector<std::string> const &link_dynamic_libs
) {
    uint32_t opt_max_pipeline_runs = 1;
    std::vector<OutFileInfo> outs;
    std::vector<std::unique_ptr<llvm::Module>> modules;

    // TODO custom targets and disallow_fp_op_fusion parameter
    llvm::TargetMachine *target_machine = codegen::setupTargetMachine(nullptr, opt_level);

    for (auto const &file_info : file_infos) {
        std::stringstream out;
        auto file = file_info.file;
        auto code = file_info.code;
        auto const tokens = token::tokenize(file, code);
        if (out_kind == CompilerOutKind::tokens) {
            printTokens(out, tokens);
            COMPILE_ALL_FILE_DONE();
        }

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
        std::vector<codegen::Error> cg_errs;
        std::vector<codegen::Warning> cg_warns;

        std::unique_ptr<ast::Block> block = parser::parse(file, tokens, &pr_errors);
        if (out_kind == CompilerOutKind::ast) {
            printAST(out, block.get());
            printErrorsAndWarnings(code_lines, pr_errors, cg_errs, cg_warns);
            COMPILE_ALL_FILE_DONE();
        }

        codegen::State state;
        codegen::Context ctx = codegen::newContext(file, &cg_errs, &cg_warns, &state);
        block->codegen(&ctx);

        codegen::moduleSetTargetMachine(ctx.module.get(), target_machine);

        if (out_kind != CompilerOutKind::llvm_ir && pr_errors.empty() && cg_errs.empty() && cg_warns.empty())
            codegen::optimizeModule(ctx.module.get(), target_machine, opt_level, opt_max_pipeline_runs);

        if (out_kind == CompilerOutKind::llvm_ir || out_kind == CompilerOutKind::optimized_llvm_ir
            || pr_errors.size() || cg_errs.size() || cg_warns.size()) {
            out << codegen::dumpIR(&ctx);
            printErrorsAndWarnings(code_lines, pr_errors, cg_errs, cg_warns);
            COMPILE_ALL_FILE_DONE();
        }

        if (out_kind == CompilerOutKind::asm_ || out_kind == CompilerOutKind::obj || out_kind == CompilerOutKind::bitcode) {
            auto format = out_kind == CompilerOutKind::obj
                ? codegen::LoweringOutKind::obj
                : (
                    out_kind == CompilerOutKind::asm_
                    ? codegen::LoweringOutKind::asm_
                    : codegen::LoweringOutKind::bitcode
                );
            std::string mc = codegen::lowerModuleToFormatNoLinking(ctx.module.get(), target_machine, format);
            out << mc;
            COMPILE_ALL_FILE_DONE();
        }

        if (out_kind == CompilerOutKind::combined_obj || out_kind == CompilerOutKind::exe) {
            modules.push_back(std::move(ctx.module));
            continue;
        }

        throw std::runtime_error("unsupported CompilerOutKind");
    }
    if (out_kind == CompilerOutKind::combined_obj || out_kind == CompilerOutKind::exe) {
        if (!outs.empty())
            throw std::runtime_error("unreachable: outs should be empty if generating an executable");

        std::optional<std::function<void(llvm::Module*)>> combined_module_callback = std::nullopt;
        if (lto_kind == LTOKind::full) {
            combined_module_callback = [&](llvm::Module *module) {
                codegen::optimizeModule(module, target_machine, opt_level, opt_max_pipeline_runs);
            };
        } else if (lto_kind == LTOKind::thin) {
            std::cerr << "thin lto not yet supported";
            throw std::runtime_error("thin lto not yet supported");
        } else if (lto_kind != LTOKind::none) {
            throw std::runtime_error("unreachable");
        }

        // TODO use lowerModuleToExeWithLTO once I have that working
        std::string out = codegen::lowerModulesToFormatNoLTO(std::move(modules), target_machine, codegen::LoweringOutKind::combined_obj, combined_module_callback).at(0);
        if (out_kind == CompilerOutKind::exe) {
            out = codegen::linkExternalLibraries(out, link_static_libs, link_dynamic_libs);
        }
        outs.push_back(OutFileInfo {.file = std::move(out_filename), .content = std::move(out)});
    }
    return outs;
}

void runTests() {
    CompilerOutKind out_kind = CompilerOutKind::asm_;
    OptLevel opt_level = OptLevel::O3;
    LTOKind lto_kind = LTOKind::none;

    for (auto const &e : std::filesystem::directory_iterator("test/code_samples")) {
        std::string path = e.path();
        std::cout << "RUNNNIG TEST " << path << std::endl << std::endl;
        std::ifstream in_file(path);
        std::stringstream content_strm;
        content_strm << in_file.rdbuf();
        std::string content(content_strm.str());
        auto file_sr = StringRef {
            .start = path.c_str(),
            .length = static_cast<uint32_t>(path.length()),
        };
        std::string out_path = std::string("test/llvmir_out") + &path.c_str()[path.find_last_of('/')] + ".out";
        std::string ir = compileAll(
            {
                SourceFileInfo {
                    .file = file_sr,
                    .code = StringRef {.start = content.c_str(), .length = static_cast<uint32_t>(content.length())}
                }
            },
            out_kind,
            opt_level,
            lto_kind,
            out_path,
            {},
            {}
        ).at(0).content;
        std::cout << "writing output to " << out_path << std::endl;
        std::ofstream out_file(out_path);
        out_file << ir;
    }

}

#define SUCCESS() {return 0;}
#define FAILURE() {return 1;}
#define INVALID_USAGE() {std::cout << invalid_usage_msg; return 1;}

#define MAP_STR_OPTLEVEL_TO_OPTLEVEL(var, from, to) case from: var = OptLevel::O##to; break;

int main(int argc, char **argv) {
    if (argc == 1) {
#ifdef RUN_TESTS_ON_EMPTY_PARAMS
        runTests();
        SUCCESS();
#else
        FAILURE();
#endif
    }

    OptLevel opt_level = OptLevel::O1;
    LTOKind lto_kind = LTOKind::none;
    CompilerOutKind out_kind = CompilerOutKind::asm_;
    std::string out_path;
    std::vector<std::string> paths;
    std::vector<std::string> link_static_libs;
    std::vector<std::string> link_dynamic_libs;

    // FIXME
    std::vector<char const*> source_files;
    char const *out_file = nullptr;
    bool prev_was_dash_o = false;
    bool dash_o_was_set = false;
    bool prev_was_dash_l = false;
    bool opt_level_has_been_set = false;
    bool prev_was_dash_I = false;
    bool prev_was_dash_J = false;
    bool emit_llvm = false;
    std::vector<char const*> user_include_paths;
    std::vector<char const*> sys_include_paths;

    for (int i = 1; i < argc; i++) {
        llvm::StringRef arg(argv[i]);
        if (arg == "--help" || arg == "-h") {
            llvm::outs() << help_msg;
            SUCCESS();
        } else if (arg == "-emit-llvm") {
            emit_llvm = true;
        } else if (arg.size() >= 2 && arg[0] == '-' && arg[1] == 'o') {
            if (prev_was_dash_o) {
                INVALID_USAGE();
            }
            prev_was_dash_o = true;

            if (arg.size() > 2) {
                arg = llvm::StringRef(argv[i] + 2);  // skip -o
                goto HANDLE_DASH_O;  // handle -o directly as if there was a space
            }
        } else if (prev_was_dash_o) {
        HANDLE_DASH_O:
            prev_was_dash_o = false;
            if (dash_o_was_set)
                INVALID_USAGE();
            if (arg != "-")
                out_file = arg.data();
            dash_o_was_set = true;
        } else if (arg.size() > 2 && arg[0] == '-' && arg[1] == 'O') {
            if (opt_level_has_been_set)
                INVALID_USAGE();
            opt_level_has_been_set = true;
            switch (arg[2]) {
                MAP_STR_OPTLEVEL_TO_OPTLEVEL(opt_level, '0', 0);
                MAP_STR_OPTLEVEL_TO_OPTLEVEL(opt_level, '1', 1);
                MAP_STR_OPTLEVEL_TO_OPTLEVEL(opt_level, '2', 2);
                MAP_STR_OPTLEVEL_TO_OPTLEVEL(opt_level, 's', s);
                MAP_STR_OPTLEVEL_TO_OPTLEVEL(opt_level, 'z', z);
                MAP_STR_OPTLEVEL_TO_OPTLEVEL(opt_level, '3', 3);
                default: INVALID_USAGE();
            }
        } else if (arg.size() >= 2 && arg[0] == '-' && arg[1] == 'l') {
            prev_was_dash_l = true;
            if (arg.size() > 2) {
                arg = llvm::StringRef(argv[i] + 2);  // skip -l
                goto HANDLE_DASH_L;  // handle -l directly as if there was a space
            }
        } else if (prev_was_dash_l) {
        HANDLE_DASH_L:
            prev_was_dash_l = false;
            // TODO I should have a link_libs, link_dynamic_libs and link_static_libs vector (1 makes linker decide which to use, 2 and 3 are guaranteed that)
            link_dynamic_libs.emplace_back(arg.data());
        } else if (arg.size() >= 2 && arg[0] == '-' && arg[1] == 'I') {
            prev_was_dash_I = true;
            if (arg.size() > 2) {
                arg = llvm::StringRef(argv[i] + 2);
                goto HANDLE_DASH_CAP_I;
            }
        } else if (prev_was_dash_I) {
        HANDLE_DASH_CAP_I:
            prev_was_dash_I = false;
            user_include_paths.emplace_back(arg.data());
        } else if (arg.size() >= 2 && arg[0] == '-' && arg[1] == 'J') {
            prev_was_dash_J = true;
            if (arg.size() > 2) {
                arg = llvm::StringRef(argv[i] + 2);
                goto HANDLE_DASH_CAP_J;
            }
        } else if (prev_was_dash_J) {
        HANDLE_DASH_CAP_J:
            prev_was_dash_J = false;
            sys_include_paths.emplace_back(arg.data());
        } else {
            if (arg[0] == '-')
                INVALID_USAGE();
            source_files.emplace_back(arg.data());
        }
    }
    
    if (!dash_o_was_set)
        out_file = "a.out";
    if (source_files.empty()) {
        llvm::outs() << "No source files provided!\n";
        FAILURE();
    }

    std::vector<SourceFileInfo> files;
    for (char const *path : source_files) {
        std::string path_string(path);
        std::ifstream in_file(path_string);
        std::stringstream content_strm;
        content_strm << in_file.rdbuf();
        std::string content(content_strm.str());

        auto file_sr = StringRef {
            .start = path,
            .length = static_cast<uint32_t>(path_string.length()),
        };
        auto content_sr = StringRef {.start = content.c_str(), .length = static_cast<uint32_t>(content.length())};
        files.push_back(SourceFileInfo {
            .file = file_sr,
            .code = content_sr,
        });
    }

    std::vector<OutFileInfo> outs = compileAll(
        files,
        out_kind,
        opt_level,
        lto_kind,
        out_path,
        link_static_libs,
        link_dynamic_libs
    );

    for (auto const &out : outs) {
        std::ofstream out_file(std::move(out.file));
        out_file << std::move(out.content);
    }

    SUCCESS();
}
