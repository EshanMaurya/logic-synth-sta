#pragma once
#include "lss/boolfunc.hpp"
#include "lss/cube.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace lss {

std::vector<Cube> generate_primes(const std::vector<Cube>& minterms);

struct PrimeImplicantChart {
    std::vector<Cube> primes;
    std::vector<Cube> minterms;
    std::vector<BitVec> covers;
    std::vector<BitVec> covered_by;

    static PrimeImplicantChart build(const std::vector<Cube>& primes, const std::vector<Cube>& on_minterms);
};

struct CoverSolution {
    std::vector<std::size_t> selected;
    std::vector<std::size_t> essential;
    bool exact = true;
    std::size_t reduced_rows = 0;
    std::size_t reduced_cols = 0;
    std::size_t petrick_peak_terms = 0;
};

CoverSolution solve_cover(const PrimeImplicantChart& chart, std::size_t petrick_term_limit = 5000);

struct MinimizeOptions {
    std::size_t petrick_term_limit = 5000;
};

struct MinimizeResult {
    std::vector<Cube> primes;
    std::vector<std::size_t> essential;
    std::vector<std::size_t> selected;
    std::vector<Cube> cover;
    bool exact = true;
    std::size_t term_count = 0;
    std::size_t literal_count = 0;
    std::size_t reduced_rows = 0;
    std::size_t reduced_cols = 0;
    std::size_t petrick_peak_terms = 0;
};

MinimizeResult minimize_exact(const BoolFunction& f, const MinimizeOptions& opts = {});

bool verify_cover(const BoolFunction& f, const std::vector<Cube>& cover);

std::string format_sop(const std::vector<Cube>& cover, const std::vector<std::string>& names);
std::string format_pos(const std::vector<Cube>& complement_cover, const std::vector<std::string>& names);

} // namespace lss
