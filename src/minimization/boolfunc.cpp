#include <algorithm>
#include "lss/boolfunc.hpp"
#include <stdexcept>
#include <unordered_set>

namespace lss {

namespace {

constexpr std::size_t kMaxMinterms = 1ULL << 26;

std::unordered_set<Cube, CubeHash> expand_set(const std::vector<Cube>& cubes) {
    std::unordered_set<Cube, CubeHash> out;
    for (const Cube& c : cubes) {
        if (c.dash_count() >= 40 || (1ULL << c.dash_count()) > kMaxMinterms)
            throw std::length_error("cube expands to too many minterms");
        if (c.is_minterm()) {
            out.insert(c);
            continue;
        }
        for (Cube& m : c.minterms()) out.insert(std::move(m));
        if (out.size() > kMaxMinterms) throw std::length_error("too many minterms");
    }
    return out;
}

std::vector<Cube> sorted(const std::unordered_set<Cube, CubeHash>& s) {
    std::vector<Cube> v(s.begin(), s.end());
    std::sort(v.begin(), v.end());
    return v;
}

} // namespace

std::vector<std::string> default_var_names(std::size_t n) {
    std::vector<std::string> names;
    for (std::size_t i = 0; i < n; ++i) {
        if (n <= 26) names.emplace_back(1, static_cast<char>('a' + i));
        else names.push_back("x" + std::to_string(i));
    }
    return names;
}

std::vector<Cube> BoolFunction::on_minterms() const { return sorted(expand_set(on)); }

std::vector<Cube> BoolFunction::dc_minterms() const {
    auto on_set = expand_set(on);
    auto dc_set = expand_set(dc);
    for (const Cube& c : on_set) dc_set.erase(c);
    return sorted(dc_set);
}

std::vector<Cube> BoolFunction::off_minterms() const {
    std::size_t n = num_vars();
    if (n > kMaxEnumVars) throw std::length_error("too many variables to enumerate the off-set");
    auto on_set = expand_set(on);
    auto dc_set = expand_set(dc);
    std::vector<Cube> off;
    for (std::uint64_t a = 0; a < (1ULL << n); ++a) {
        Cube m = Cube::from_minterm(n, a);
        if (on_set.count(m) || dc_set.count(m)) continue;
        off.push_back(std::move(m));
    }
    return off;
}

BoolFunction BoolFunction::complement() const {
    BoolFunction g;
    g.name = name + "'";
    g.var_names = var_names;
    g.on = off_minterms();
    g.dc = dc_minterms();
    return g;
}

} // namespace lss
