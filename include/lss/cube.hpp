#pragma once
#include "lss/bitvec.hpp"
#include <optional>
#include <string>
#include <vector>

namespace lss {

enum class Lit : std::uint8_t { Zero = 0, One = 1, DontCare = 2 };

class Cube {
public:
    Cube() = default;
    explicit Cube(std::size_t num_vars) : value_(num_vars), mask_(num_vars) {}

    static Cube from_string(const std::string& s);
    static Cube from_minterm(std::size_t num_vars, std::uint64_t index);
    static Cube universe(std::size_t num_vars);

    std::size_t num_vars() const { return value_.size(); }

    Lit get(std::size_t var) const {
        if (mask_.test(var)) return Lit::DontCare;
        return value_.test(var) ? Lit::One : Lit::Zero;
    }
    void set(std::size_t var, Lit l);

    std::size_t literal_count() const { return num_vars() - mask_.count(); }
    std::size_t dash_count() const { return mask_.count(); }
    std::size_t ones_count() const { return value_.count(); }
    bool is_minterm() const { return mask_.none(); }

    bool covers(const Cube& o) const;
    bool contains_assignment(std::uint64_t assignment) const;
    bool contains_assignment(const BitVec& assignment) const;
    bool intersects(const Cube& o) const;
    std::optional<Cube> intersect(const Cube& o) const;
    std::size_t distance(const Cube& o) const;
    std::optional<Cube> merge_adjacent(const Cube& o) const;

    std::vector<Cube> minterms() const;
    std::uint64_t minterm_count_log2() const { return dash_count(); }

    std::string to_string() const;
    std::string to_product(const std::vector<std::string>& names) const;
    std::string to_sum(const std::vector<std::string>& names) const;

    const BitVec& value() const { return value_; }
    const BitVec& mask() const { return mask_; }

    bool operator==(const Cube& o) const { return value_ == o.value_ && mask_ == o.mask_; }
    bool operator!=(const Cube& o) const { return !(*this == o); }
    bool operator<(const Cube& o) const {
        if (mask_ != o.mask_) return mask_ < o.mask_;
        return value_ < o.value_;
    }

    std::size_t hash() const { return value_.hash() * 31 + mask_.hash(); }

private:
    BitVec value_;
    BitVec mask_;
};

struct CubeHash {
    std::size_t operator()(const Cube& c) const { return c.hash(); }
};

} // namespace lss
