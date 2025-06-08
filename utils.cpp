#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "utfcpp/source/utf8/unchecked.h"

// ( - bracket/parenthesis
// [ - square bracket/bracket
// { - curly brace
// " - neutral double quotes
// ' - neutral single quotes

enum class TokenT {
    IDENTIFIER,
    NUMBER,
    STRING_LITERAL,
    OPEN_PAR,
    CLOSE_PAR,
    OPEN_SQBR,
    CLOSE_SQBR,
    DBL_QUOTE,
    SINGLE_QUOTE,
    COMMA_SEP,
    MEMBER_ACCESS,
    UNKNOWN
};

enum class ErrorReason {
    UNEXPECTED_SYMBOL,
    MISSING_CLOSE_PAR,
    MISSING_CLOSE_SQBR,
    MISSING_DBL_QUOTE,
    MISSING_OPEN_PAR,
    MISSING_OPEN_SQBR,
    MISSING_CLOSE_PAR_MULTILINE,
    MISSING_CLOSE_SQBR_MULTILINE,
    MISSING_CLOSE_DBL_QUOTE_MULTILINE
};

constexpr std::string_view error_verbose(const ErrorReason code) {
    switch(code) {
        case ErrorReason::UNEXPECTED_SYMBOL:
            return "UNEXPECTED_SYMBOL";
        case ErrorReason::MISSING_CLOSE_PAR:
            return "MISSING_CLOSE_PAR";
        case ErrorReason::MISSING_CLOSE_SQBR:
            return "MISSING_CLOSE_SQBR";
        case ErrorReason::MISSING_DBL_QUOTE:
            return "MISSING_DBL_QUOTE";
        case ErrorReason::MISSING_OPEN_PAR:
            return "MISSING_OPEN_PAR";
        case ErrorReason::MISSING_OPEN_SQBR:
            return "MISSING_OPEN_SQBR";
        case ErrorReason::MISSING_CLOSE_PAR_MULTILINE:
            return "MISSING_CLOSE_PAR_MULTILINE";
        case ErrorReason::MISSING_CLOSE_SQBR_MULTILINE:
            return "MISSING_CLOSE_SQBR_MULTILINE";
        case ErrorReason::MISSING_CLOSE_DBL_QUOTE_MULTILINE:
            return "MISSING_CLOSE_DBL_QUOTE_MULTILINE";
        default:
            return "UNKNOWN ERROR";
    }
}

struct Token {
    public:
    const TokenT type;
    const std::string_view value;
    const size_t pos;
};

class Level1 {
    private:
    struct Parenth {
        const TokenT type;
        const size_t pos;
    };
    std::vector<Parenth> parity_check_helper;

    void get_iden(char32_t symbol, const auto begin, auto& iter, const auto& iter_end, size_t& index, std::vector<Token>& tokens) {
        const auto begin_pos{index};
        auto cop = iter;

        while (iter!=iter_end) { // VERY QUESTIONABLE
            index++;
            symbol=utf8::unchecked::next(iter);
            switch (symbol) {
                case U' ':
                case U'\t':
                case U'(':
                case U')':
                case U'[':
                case U']':
                case U'"':
                case U',':
                case U'.':
                case U'\\':
                    iter = cop;
                    tokens.push_back({.type=TokenT::IDENTIFIER, .value=std::string_view(begin,iter), .pos=begin_pos});
                    return;
                default:
                    cop = iter;
                    break;
            }
        }
    }

    public:
    struct Error {
        ErrorReason reason;
        size_t pos;
    };
    std::vector<Token> tokens;

    std::optional<Error> tokenize(const std::string& input) {
        if (input.length() == 0) {
            return {};
        }
        bool has_decimal_sep = false;
        bool has_exponent = false;

        size_t i{1};
        auto iter = input.begin();
        const auto iter_end = input.end();
        //utf8::unchecked::next(octet_iterator &it)
        char32_t symbol;
        while (iter!=iter_end) {
            const auto begin{iter};
            symbol = utf8::unchecked::next(iter);
            switch (symbol) {
                case U' ':
                case U'\t':
                    i++;
                    break;
                case U'(':
                    parity_check_helper.push_back({.type=TokenT::OPEN_PAR, .pos=i});
                    tokens.push_back({.type=TokenT::OPEN_PAR, .value=std::string_view("(",1), .pos=i});
                    i++;
                    break;
                case U')':
                    tokens.push_back({.type=TokenT::CLOSE_PAR, .value=std::string_view(")",1), .pos=i});
                    if (!parity_check_helper.empty() && parity_check_helper.back().type == TokenT::OPEN_PAR) {
                        parity_check_helper.pop_back();
                    }
                    else {
                        return Error{.reason=ErrorReason::MISSING_OPEN_PAR, .pos=i};
                    }
                    i++;
                    break;
                case U'[':
                    parity_check_helper.push_back({.type=TokenT::OPEN_SQBR, .pos=i});
                    tokens.push_back({.type=TokenT::OPEN_SQBR, .value=std::string_view("[",1), .pos=i});
                    i++;
                    break;
                case U']':
                    tokens.push_back({.type=TokenT::CLOSE_SQBR, .value=std::string_view("]",1), .pos=i});
                    if (!parity_check_helper.empty() && parity_check_helper.back().type == TokenT::OPEN_SQBR) {
                        parity_check_helper.pop_back();
                    }
                    else {
                        return Error{.reason=ErrorReason::MISSING_OPEN_SQBR, .pos=i};
                    }
                    i++;
                    break;
                case U'"':
                    tokens.push_back({.type=TokenT::DBL_QUOTE, .value=std::string_view("\"",1), .pos=i});
                    i++;
                    if (iter==iter_end) {
                        return Error{.reason=ErrorReason::MISSING_DBL_QUOTE, .pos=i};
                    }
                    else {
                        const auto _begin = iter;
                        const size_t begin_pos{i};
                        auto last = iter;
                        while (iter!=iter_end) {
                            last = iter;
                            symbol = utf8::unchecked::next(iter);
                            if (symbol == U'\\') {
                                i++;
                                if (iter == iter_end) {
                                    break;
                                }
                                //last = iter;
                                symbol = utf8::unchecked::next(iter);
                            }
                            else if (symbol == U'"') {
                                break;
                            }
                            i++;
                        }
                        tokens.push_back({.type=TokenT::STRING_LITERAL, .value=std::string_view(_begin, last), .pos=begin_pos});
                    }
                    if (iter!=iter_end || (iter==iter_end && symbol==U'"')) {
                        tokens.push_back({.type=TokenT::DBL_QUOTE, .value=std::string_view("\"",1), .pos=i});
                        i++;
                    }
                    else {
                        return Error{.reason=ErrorReason::MISSING_DBL_QUOTE, .pos=i};
                    }
                    break;
                case U',':
                    tokens.push_back({.type=TokenT::COMMA_SEP, .value=std::string_view(",",1), .pos=i});
                    i++;
                    break;
                case U'.':
                    if (iter==iter_end) {
                        tokens.push_back({.type=TokenT::MEMBER_ACCESS, .value=std::string_view(".",1), .pos=i});
                        i++;
                        break;
                    }
                    else {
                        //auto copy = iter;
                        if (U'0' <= (symbol=utf8::unchecked::peek_next(iter)) && symbol <= U'9') {
                            has_decimal_sep = true;
                        }
                        else {
                            tokens.push_back({.type=TokenT::MEMBER_ACCESS, .value=std::string_view(".",1), .pos=i});
                            i++;
                            break;
                        }
                    }
                    [[fallthrough]];
                case U'0':
                case U'1':
                case U'2':
                case U'3':
                case U'4':
                case U'5':
                case U'6':
                case U'7':
                case U'8':
                case U'9':
                    {
                        const auto begin_pos = i;
                        auto cop = iter;
                        while (iter!=iter_end) {
                            i++;
                            if (U'0' <= (symbol=utf8::unchecked::next(iter)) && symbol <= U'9') {
                                cop = iter;
                            }
                            else if (symbol == U'.') {
                                if (has_decimal_sep || has_exponent) {
                                    return Error{ .reason=ErrorReason::UNEXPECTED_SYMBOL, .pos=i };
                                }
                                else {
                                    has_decimal_sep = true;
                                }
                            }
                            else if ((symbol == U'e' || symbol == U'E') && !has_exponent) {
                                has_exponent = true;
                            }
                            else {
                                iter = cop;
                                break;
                            }
                        }
                        tokens.push_back({.type=TokenT::NUMBER, .value=std::string_view(begin,iter), .pos=begin_pos});
                        //continue;
                    }
                    break;
                case U'\\':
                    return Error{.reason=ErrorReason::UNEXPECTED_SYMBOL, .pos=i};
                default:
                    get_iden(symbol, begin, iter, iter_end, i, tokens);
                    // if (std::isalpha(symbol,loc)) {
                    //     const auto begin_pos{i};
                    //     auto cop = iter;
                    //     while (iter!=iter_end) { // VERY QUESTIONABLE
                    //         i++;
                    //         symbol=utf8::unchecked::next(iter);
                    //         if (std::isalnum(symbol, loc)) {
                    //             cop = iter;
                    //         }
                    //         else {
                    //             iter = cop;
                    //             break;
                    //         }
                    //     }
                    //     tokens.push_back({.type=TokenT::IDENTIFIER, .value=std::string_view(begin,iter), .pos=begin_pos});
                    // }
                    // else if (std::ispunct(symbol, loc)) { // ALSO KINDA WEIRD
                    //     tokens.push_back({.type=TokenT::IDENTIFIER, .value=std::string_view(begin,iter), .pos=i});
                    //     i++;
                    // }
                    // else {
                    //     tokens.push_back({.type=TokenT::UNKNOWN, .value=std::string_view(begin,iter), .pos=i});
                    //     i++;
                    // }
            }
        }

        if (parity_check_helper.empty()) {
            return {};
        }
        else {
            const auto last_parenth = parity_check_helper.back();
            switch (last_parenth.type) {
                case TokenT::OPEN_PAR:
                    return Error{.reason = ErrorReason::MISSING_CLOSE_PAR, .pos = i};
                case TokenT::OPEN_SQBR:
                    return Error{.reason = ErrorReason::MISSING_CLOSE_SQBR, .pos = i};
                default:
                    return Error{.reason = ErrorReason::UNEXPECTED_SYMBOL, .pos = last_parenth.pos};
            };
        }
    }
};

class Level2 {
    private:
    struct Parenth {
        const TokenT type;
        const size_t line_idx;
        const size_t pos;
        const size_t nested_depth{0};
    };
    
    std::vector<Parenth> parity_check_helper;
    std::vector<size_t> quote_d;
    size_t invocations{0};

    public:
    struct Error {
        const ErrorReason reason;
        const size_t line_idx;
        const size_t pos;
    };

    void reset() {
        parity_check_helper.clear();
        invocations = 0;
        quote_d.clear();
    }

    [[nodiscard]] std::optional<Error> parse(const Token& _input, const size_t line_idx) {
        const auto& input = _input.value;
        auto iter = input.begin();
        const auto iter_end = input.end();
        const auto base_offset = _input.pos;
        if (input.length() == 0) {
            return {};
        }

        size_t i{0};
        char32_t symbol;
        while (iter!=iter_end) {
            symbol = utf8::unchecked::next(iter);
            switch (symbol) {
                case U'(':
                    parity_check_helper.push_back({.type=TokenT::OPEN_PAR, .line_idx=line_idx, .pos=base_offset+i});
                    i++;
                    break;
                case U')':
                    if (!parity_check_helper.empty() && parity_check_helper.back().type == TokenT::OPEN_PAR) {
                        parity_check_helper.pop_back();
                    }
                    else {
                        return Error{.reason=ErrorReason::MISSING_OPEN_PAR, .line_idx=line_idx, .pos=base_offset+i};
                    }
                    i++;
                    break;
                case U'[':
                    parity_check_helper.push_back({.type=TokenT::OPEN_SQBR, .line_idx=line_idx, .pos=base_offset+i});
                    i++;
                    break;
                case U']':
                    if (!parity_check_helper.empty() && parity_check_helper.back().type == TokenT::OPEN_SQBR) {
                        parity_check_helper.pop_back();
                    }
                    else {
                        return Error{.reason=ErrorReason::MISSING_OPEN_SQBR, .line_idx=line_idx, .pos=base_offset+i};
                    }
                    i++;
                    break;
                case U'\\':
                    if (iter!=iter_end) {
                        i++;
                        size_t depth = 1;
                        //symbol = utf8::unchecked::next(iter);
                        while (iter!=iter_end && (symbol=utf8::unchecked::next(iter))==U'\\') {
                            depth+=1;
                            //i++;
                        }
                        i = i - 1 + depth;
                        //symbol = input[i];
                        if (symbol == U'"') {
                            depth = (depth - 1) / 2;
                            if (quote_d.empty() || quote_d.back()<depth) {
                                parity_check_helper.push_back({.type=TokenT::DBL_QUOTE, .line_idx=line_idx, .pos=base_offset+i, .nested_depth=depth});
                                quote_d.push_back(depth);
                            }
                            else if (quote_d.back()==depth) {
                                if (!parity_check_helper.empty() && parity_check_helper.back().type==TokenT::DBL_QUOTE && parity_check_helper.back().nested_depth==depth) {
                                    parity_check_helper.pop_back();
                                    quote_d.pop_back();
                                }
                                else {
                                    return Error{.reason=ErrorReason::MISSING_DBL_QUOTE, .line_idx=line_idx, .pos=base_offset+i};
                                }
                            }
                            else {
                                return Error{.reason=ErrorReason::MISSING_DBL_QUOTE, .line_idx=line_idx, .pos=base_offset+i};
                            }
                        }
                        i++;
                    }
                    break;
                default:
                    i++;
            }
        }
        invocations++;
        return {};
    }

    [[nodiscard]] std::optional<Error> final_checks() const {
        if (parity_check_helper.empty()) {
            return {};
        }
        else {
            const auto last_parenth = parity_check_helper.back();
            switch (last_parenth.type) {
                case TokenT::OPEN_PAR:
                    if (invocations == 0) {
                        return Error{.reason=ErrorReason::MISSING_CLOSE_PAR, .line_idx=last_parenth.line_idx, .pos=last_parenth.pos};
                    } else {
                        return Error{.reason = ErrorReason::MISSING_CLOSE_PAR_MULTILINE, .line_idx=last_parenth.line_idx, .pos=last_parenth.pos};
                    }
                case TokenT::OPEN_SQBR:
                    if (invocations == 0) {
                        return Error{.reason = ErrorReason::MISSING_CLOSE_SQBR, .line_idx=last_parenth.line_idx, .pos = last_parenth.pos};
                    } else {
                        return Error{.reason = ErrorReason::MISSING_CLOSE_SQBR_MULTILINE, .line_idx=last_parenth.line_idx, .pos = last_parenth.pos};
                    }
                case TokenT::DBL_QUOTE:
                    if (invocations == 0) {
                        return Error{.reason = ErrorReason::MISSING_DBL_QUOTE, .line_idx=last_parenth.line_idx, .pos = last_parenth.pos};
                    } else {
                        return Error{.reason = ErrorReason::MISSING_CLOSE_DBL_QUOTE_MULTILINE, .line_idx=last_parenth.line_idx, .pos = last_parenth.pos};
                    }
                default:
                    return Error{.reason = ErrorReason::UNEXPECTED_SYMBOL, .line_idx=last_parenth.line_idx, .pos = last_parenth.pos};
            }
        }
    }
};