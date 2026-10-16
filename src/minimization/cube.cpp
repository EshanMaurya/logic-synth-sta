#include "lss/cube.hpp"
#include <bit>
#include <stdexcept>

namespace lss {

Cube Cube::from_string(const std::string& s) {
    Cube c(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        switch (s[i]) {
            case '0': c.set(i, Lit::Zero); break;
            case '1': c.set(i, Lit::One); break;
            case '-':
            case '2':
            case 'x':
            case 'X': c.set(i, Lit::DontCare); break;
            default: throw std::invalid_argument(std::string("invalid cube character '") + s[i] + "'");
        }
    }
    return c;
}

Cube Cube::from_minterm(std::size_t num_vars, std::uint64_t index) {
    if (num_vars > 64) throw std::invalid_argument("minterm index limited to 64 variables");
    Cube c(num_vars);
    for (std::size_t i = 0; i < num_vars; ++i) {
        bool bit = (index >> (num_vars - 1 - i)) & 1ULL;
        if (bit) c.value_.set(i);
    }
    return c;
}

Cube Cube::universe(std::size_t num_vars) {
    Cube c(num_vars);
    for (std::size_t i = 0; i < num_vars; ++i) c.mask_.set(i);
    return c;
}

void Cube::set(std::size_t var, Lit l) {
    switch (l) {
        case Lit::Zero: mask_.reset(var); value_.reset(var); break;
        case Lit::One: mask_.reset(var); value_.set(var); break;
        case Lit::DontCare: mask_.set(var); value_.reset(var); break;
    }
}

bool Cube::covers(const Cube& o) const {
    const auto& tm = mask_.words();
    const auto& om = o.mask_.words();
    const auto& tv = value_.words();
    const auto& ov = o.value_.words();
    for (std::size_t i = 0; i < tm.size(); ++i) {
        std::uint64_t care_here = ~tm[i];
        if (care_here & om[i]) return false;
        if ((tv[i] ^ ov[i]) & care_here) return false;
    }
    return true;
}

bool Cube::contains_assignment(const BitVec& a) const {
    const auto& tm = mask_.words();
    const auto& tv = value_.words();
    const auto& av = a.words();
    for (std::size_t i = 0; i < tm.size(); ++i)
        if ((tv[i] ^ av[i]) & ~tm[i]) return false;
    return true;
}

bool Cube::contains_assignment(std::uint64_t assignment) const {
    std::size_t n = num_vars();
    for (std::size_t i = 0; i < n; ++i) {
        if (mask_.test(i)) continue;
        bool bit = (assignment >> (n - 1 - i)) & 1ULL;
        if (bit != value_.test(i)) return false;
    }
    return true;
}

std::size_t Cube::distance(const Cube& o) const {
    std::size_t d = 0;
    const auto& tm = mask_.words();
    const auto& om = o.mask_.words();
    const auto& tv = value_.words();
    const auto& ov = o.value_.words();
    for (std::size_t i = 0; i < tm.size(); ++i) {
        std::uint64_t both_care = ~tm[i] & ~om[i];
        d += static_cast<std::size_t>(std::popcount((tv[i] ^ ov[i]) & both_care));
    }
    return d;
}

bool Cube::intersects(const Cube& o) const { return distance(o) == 0; }

std::optional<Cube> Cube::intersect(const Cube& o) const {
    if (num_vars() != o.num_vars() || distance(o) != 0) return std::nullopt;
    Cube r(num_vars());
    for (std::size_t i = 0; i < num_vars(); ++i) {
        Lit a = get(i);
        Lit b = o.get(i);
        r.set(i, a == Lit::DontCare ? b : a);
    }
    return r;
}

std::optional<Cube> Cube::merge_adjacent(const Cube& o) const {
    if (mask_ != o.mask_) return std::nullopt;
    BitVec diff = value_;
    diff ^= o.value_;
    if (diff.count() != 1) return std::nullopt;
    Cube r = *this;
    std::size_t v = diff.find_first();
    r.mask_.set(v);
    r.value_.reset(v);
    return r;
}

std::vector<Cube> Cube::minterms() const {
    std::size_t d = dash_count();
    if (d >= 40) throw std::length_error("cube expands to too many minterms");
    std::vector<std::size_t> dash_vars;
    for (std::size_t i = 0; i < num_vars(); ++i)
        if (mask_.test(i)) dash_vars.push_back(i);
    std::vector<Cube> out;
    out.reserve(static_cast<std::size_t>(1) << d);
    for (std::uint64_t m = 0; m < (1ULL << d); ++m) {
        Cube c = *this;
        for (std::size_t k = 0; k < d; ++k) {
            c.mask_.reset(dash_vars[k]);
            if ((m >> k) & 1ULL) c.value_.set(dash_vars[k]);
        }
        out.push_back(std::move(c));
    }
    return out;
}

std::string Cube::to_string() const {
    std::string s(num_vars(), '-');
    for (std::size_t i = 0; i < num_vars(); ++i) {
        Lit l = get(i);
        s[i] = l == Lit::DontCare ? '-' : (l == Lit::One ? '1' : '0');
    }
    return s;
}

static bool all_single_char(const std::vector<std::string>& names) {
    for (const auto& n : names)
        if (n.size() != 1) return false;
    return true;
}

std::string Cube::to_product(const std::vector<std::string>& names) const {
    bool compact = all_single_char(names);
    std::string out;
    bool first = true;
    for (std::size_t i = 0; i < num_vars(); ++i) {
        Lit l = get(i);
        if (l == Lit::DontCare) continue;
        if (!first && !compact) out += "*";
        out += names[i];
        if (l == Lit::Zero) out += "'";
        first = false;
    }
    return out.empty() ? "1" : out;
}

std::string Cube::to_sum(const std::vector<std::string>& names) const {
    std::string out;
    bool first = true;
    for (std::size_t i = 0; i < num_vars(); ++i) {
        Lit l = get(i);
        if (l == Lit::DontCare) continue;
        if (!first) out += "+";
        out += names[i];
        if (l == Lit::One) out += "'";
        first = false;
    }
    if (out.empty()) return "0";
    return out.find('+') != std::string::npos ? "(" + out + ")" : out;
}

} // namespace lss
