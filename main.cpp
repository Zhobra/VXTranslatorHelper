#include <fstream>
#include "fmt/base.h"
#include "fmt/ostream.h"
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include "utils.cpp"
#ifdef _WIN32
#include <windows.h>
#include <memory>
#endif


struct LexErr {
    size_t line;
    Level1::Error err;
};

#ifdef _WIN32
std::wstring widen(std::string_view input) {
    if (input.length() > (int)((~0u)>>1)) {
        return L"";
    }
    auto new_size = ::MultiByteToWideChar(CP_UTF8, 0, input.data(), (int)input.length(), NULL, 0);
    std::wstring res(new_size,0);
    ::MultiByteToWideChar(CP_UTF8, 0, input.data(), (int)input.length(), res.data(), new_size);
    return res;
}

std::string narrow(std::wstring_view input) {
    if (input.length() > (int)((~0u)>>1)) {
        return "";
    }
    auto new_size = ::WideCharToMultiByte(CP_UTF8, 0, input.data(), (int)input.length(), NULL, 0, NULL, NULL);
    std::string res(new_size,0);
    ::WideCharToMultiByte(CP_UTF8, 0, input.data(), (int)input.length(), res.data(), new_size, NULL, NULL);
    return res;
}
#endif

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
        fmt::println(error_output, "File \"{}\"", narrow(file_path.wstring()));
        for (const auto& err : errs) {
            fmt::println(error_output, "Error at line {}, symbol #{}, reason \"{}\"", err.line, err.err.pos, error_verbose(err.err.reason));
        }
    }
    if (inner_errs.size() > 0) {
        fmt::println(inner_error_output, "  ----------");
        
        fmt::println(inner_error_output, "File \"{}\"", narrow(file_path.wstring()));
        for (const auto& err : inner_errs) {
            fmt::println(inner_error_output, "Inner error at line {}, symbol #{}, reason \"{}\"", err.line_idx, err.pos, error_verbose(err.reason));
        }
    }
}

bool compare_extension(const std::filesystem::path& file_path) {
#ifdef _WIN32
    //return file_path.extension().wstring() == L".txt";
#else
    //return file_path.extension().string() == ".txt";
#endif
    return file_path.extension().u8string() == u8".txt";
}

int main(int argc, char* argv[]) {
    const std::string out_err{"out_err.log"};
    const std::string out_inner_err{"out_inn_err.log"};
    
#ifdef _WIN32
    const auto deleter = [](wchar_t **ptr) { LocalFree(ptr); };
    using wstr_arr_unique_ptr = std::unique_ptr<wchar_t *[], decltype(deleter)>;
    auto wargv = wstr_arr_unique_ptr(CommandLineToArgvW(GetCommandLineW(), &argc), deleter);
    std::vector<std::string> _converted;
    std::vector<char*> wargv2argv;
    
    if (wargv==NULL) {
        fmt::println("Windows argument conversion failure (please don't hurt me)...");
        fmt::println("The app will not work... Probably. But we will still try.");
        //return -1;
    }
    else {
        _converted.reserve(argc);
        wargv2argv.reserve(argc);
        for (int i{0}; i < argc; i++) {
            _converted.push_back(narrow(wargv[i]));
        }
        for (auto& i: _converted) {
            wargv2argv.push_back(i.data());
        }
        argv = wargv2argv.data();
    }
#endif

    if (argc < 2) {
        auto name = "CHECKER.exe";
        if (argc==1) {
            name = argv[0];
        }
        fmt::println("Usage:\n\t{} FILE_OR_DIRECTORY_TO_CHECK", name);
        fmt::println("Expect the output in files \"{}\" and \"{}\"\n", out_err, out_inner_err);
        return 1;
    }
    
    std::ofstream error_output;
    std::ofstream inner_error_output;
    for (int argn{1}; argn < argc; argn++) {
        std::filesystem::path in{
#ifdef _WIN32
            wargv[argn]
#else
            argv[argn]
#endif
        };
        
        if (!std::filesystem::exists(in)) {
            fmt::println("\"{}\" does not exist...", argv[argn]);
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
                if (!compare_extension(dirEntry.path())) {
                    continue;
                }
                std::ifstream input(dirEntry.path(),std::ios::binary);
                if (!input.is_open()) {
                    fmt::println("Cannot open \"{}\".", narrow(in.wstring()));
                    continue;
                }
                process_file(input, error_output, inner_error_output, dirEntry.path());
            }
        }
        else { // if input is a single file
            if (!std::filesystem::is_regular_file(in)) {
                fmt::println("\"{}\" is a funny file.", argv[argn]);
                return 4;
            }

            if (!compare_extension(in)) {
                fmt::println("Input file seems to be something other than a \".txt\"");
                fmt::println("You should not rely on the result too much.");
            }
            std::ifstream input(in,std::ios::binary);
            if (!input.is_open()) {
                fmt::println("Cannot open \"{}\".", narrow(in.wstring()));
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