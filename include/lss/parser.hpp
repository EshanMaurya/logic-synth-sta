#pragma once
#include "lss/boolfunc.hpp"
#include <istream>
#include <stdexcept>
#include <string>
#include <vector>

namespace lss {

class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& msg) : std::runtime_error(msg) {}
};

std::vector<BoolFunction> parse_pla(std::istream& in);
std::vector<BoolFunction> parse_pla_string(const std::string& text);

BoolFunction parse_minterm_list(const std::string& text, const std::vector<std::string>& vars = {});

BoolFunction parse_truth_string(const std::string& bits, const std::vector<std::string>& vars = {});

BoolFunction parse_expression(const std::string& expr, const std::string& dc_expr = "",
                              const std::vector<std::string>& vars = {});

std::vector<BoolFunction> parse_file(const std::string& path, const std::vector<std::string>& vars = {});

std::vector<std::string> split_var_list(const std::string& csv);

} // namespace lss
