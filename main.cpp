#include <fstream>
#include "fmt/base.h"
#include "fmt/ostream.h"
#include <filesystem>
#include <string>
#include <vector>
#include "utils.cpp"

struct LexErr {
    size_t line;
    Level1::Error err;
};

void process_file(std::istream& input, std::ostream& error_output, std::ostream& inner_error_output, const std::filesystem::path& file_path) {
    std::vector<LexErr> errs;
    std::vector<Level2::Error> inner_errs;
    std::string line;
    line.reserve(500);
    size_t counter{1};
    Level2 SP;
    while (!input.eof()) {
        Level1 L;
        std::getline(input, line, '\n');
        if (line.ends_with('\r')) { line.pop_back(); }

        const auto res = L.tokenize(line);
        
        if (res.has_value()) {
            errs.push_back({counter, res.value()});
        }
        else if (!L.tokens.empty()) {
            if (L.tokens.front().type==TokenT::IDENTIFIER && L.tokens.front().value=="ScriptMore") {
                for (const auto& tok: L.tokens) {
                    if (tok.type==TokenT::STRING_LITERAL) {
                        const auto _res = SP.parse(tok, counter);
                        if (_res.has_value()) {
                            inner_errs.push_back(_res.value());
                        }
                    }
                }
            }
            else {
                {
                    const auto _res = SP.final_checks();
                    if (_res.has_value()) {
                        inner_errs.push_back(_res.value());
                    }
                }
                SP.reset();
                for (const auto& tok : L.tokens) {
                    if (tok.type==TokenT::STRING_LITERAL) {
                        const auto _res = SP.parse(tok, counter);
                        if (_res.has_value()) {
                            inner_errs.push_back(_res.value());
                        }
                    }
                }
            }
        }
        counter++;
    }
    const auto res = SP.final_checks();
    if (res.has_value()) {
        inner_errs.push_back(res.value());
    }
    SP.reset();

    if (errs.size() > 0) {
        fmt::println(error_output, "---------------------------------------");
        fmt::println(error_output, "File \"{}\"", file_path.string());
        for (const auto& err : errs) {
            fmt::println(error_output, "Error at line {}, symbol #{}, reason \"{}\"", err.line, err.err.pos, error_verbose(err.err.reason));
        }
    }
    if (inner_errs.size() > 0) {
        fmt::println(inner_error_output, "  ----------");
        
        fmt::println(inner_error_output, "File \"{}\"", file_path.string());
        for (const auto& err : inner_errs) {
            fmt::println(inner_error_output, "Inner error at line {}, symbol #{}, reason \"{}\"", err.line_idx, err.pos, error_verbose(err.reason));
        }
    }
}

int main(int argc, char* argv[]) {
    const std::string out_err{"out_err.log"};
    const std::string out_inner_err{"out_inn_err.log"};
    if (argc < 2) {
        const char* name = "CHECKER.exe";
        if (argc == 1) {
            name = argv[0];
        }
        fmt::println("Usage:\n\t{} FILE_OR_DIRECTORY_TO_CHECK", name);
        fmt::println("Expect the output in files \"{}\" and \"{}\"", out_err, out_inner_err);
        return 1;
    }
    
    std::ofstream error_output;
    std::ofstream inner_error_output;
    for (int argn{1}; argn < argc; argn++) {
        std::filesystem::path in(argv[argn]);
        
        if (!std::filesystem::exists(in)) {
            fmt::println("\"{}\" does not exist...",in.string());
            return -1;
        }

        if (std::filesystem::is_directory(in)) {
            if (!error_output.is_open()) {
                error_output.open(out_err);
                if (!error_output.is_open()) {
                    fmt::println("Cannot open output file for errors!");
                    return 2;
                }
            }

            if (!inner_error_output.is_open()) {
                inner_error_output.open(out_inner_err);
                if (!inner_error_output.is_open()) {
                    fmt::println("Cannot open output file for other errors!");
                    return 3;
                }
            }

            for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(in)) {
                if (!dirEntry.is_regular_file()) {
                    continue;
                }

                if (!dirEntry.path().u8string().ends_with(std::u8string{u8".txt"})) {
                    continue;
                }
                
                std::ifstream input(dirEntry.path(),std::ios::binary);
                if (!input.is_open()) {
                    fmt::println("Cannot open \"{}\".", in.string());
                    continue;
                }

                process_file(input, error_output, inner_error_output, dirEntry.path());
            }
        }
        else { // if input is a single file
            if (!std::filesystem::is_regular_file(in)) {
                fmt::println("\"{}\" is a funny file.", in.string());
                return 4;
            }

            if (!in.u8string().ends_with(std::u8string{u8".txt"})) {
                fmt::println("Input file seems to be something other than a \".txt\"");
                fmt::println("You should not rely on the result too much.");
            }
            
            std::ifstream input(in,std::ios::binary);
            if (!input.is_open()) {
                fmt::println("Cannot open \"{}\".", in.string());
                return 5;
            }

            if (!error_output.is_open()) {
                error_output.open(out_err);
                if (!error_output.is_open()) {
                    fmt::println("Cannot open output file for errors!");
                    return 2;
                }
            }

            if (!inner_error_output.is_open()) {
                inner_error_output.open(out_inner_err);
                if (!inner_error_output.is_open()) {
                    fmt::println("Cannot open output file for other errors!");
                    return 3;
                }
            }

            process_file(input, error_output, inner_error_output, in);
        }
    }

    return 0;
}