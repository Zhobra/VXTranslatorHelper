#include <string>
#include "fmt/base.h"
#include <vector>
#include "utils.cpp"

int main() {
    
    
    std::string test_input1 = "Display Name = \"Harpy Tower 2F\"";
    std::string test_input1_invalid_1 = "Display Name = \"Harpy Tower 2F";
    std::string test_input1_invalid_2 = "Display Name = Harpy Tower 2F\"";
    
    std::vector<std::string> test_cases = {
    {"Nickname = \"Apprentice Hero\""},
    {"      PlaySE([\"RPG::SE(@name=\\\"Item1\\\", @pitch=100, @volume=80)\"])"},
    {"    \"\""},
    {"    \""},
    {"    \"asdas"},
    {"    asdas\""},
    {"    as\\\"das\""}
    };

    test_cases.push_back(test_input1);
    
    for (size_t i{0}; i < test_cases.size(); ++i) {
        const auto& _case = test_cases[i];
        fmt::println("{}",_case);
        Level1 L;
        auto res = L.tokenize(_case);
        fmt::println("");
        if (res.has_value()) {
            fmt::println("Test {}: Error at {} (reason {})",i,res->pos,error_verbose(res->reason));
        }
        else {
            fmt::print("Tokens: ");
            for (const auto& j : L.tokens) {
                fmt::print("\"{}\", ",j.value);
            }
        }
        fmt::println("");
        fmt::println("---------------------------");
    }
    
    
    return 0;
}