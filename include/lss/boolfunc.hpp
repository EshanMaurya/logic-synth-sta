#pragma once
#include "lss/cube.hpp"
#include <string>
#include <vector>

namespace lss {

constexpr std::size_t kMaxEnumVars = 24;

struct BoolFunction {
    std::string name = "f";
    std::vector<std::string> var_names;
    std::vector<Cube> on;
    std::vector<Cube> dc;

    std::size_t num_vars() const { return var_names.size(); }

    std::vector<Cube> on_minterms() const;
    std::vector<Cube> dc_minterms() const;
    std::vector<Cube> off_minterms() const;
    BoolFunction complement() const;
};

std::vector<std::string> default_var_names(std::size_t n);

} // namespace lss
