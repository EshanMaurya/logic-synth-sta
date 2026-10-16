#include "lss/minimize.hpp"
#include <algorithm>
#include <unordered_set>

namespace lss {

MinimizeResult minimize_exact(const BoolFunction& f, const MinimizeOptions& opts) {
    MinimizeResult res;
    std::vector<Cube> on = f.on_minterms();
    if (on.empty()) return res;

    std::vector<Cube> all = on;
    std::vector<Cube> dc = f.dc_minterms();
    all.insert(all.end(), dc.begin(), dc.end());

    res.primes = generate_primes(all);
    PrimeImplicantChart chart = PrimeImplicantChart::build(res.primes, on);
    CoverSolution sol = solve_cover(chart, opts.petrick_term_limit);

    res.exact = sol.exact;
    res.reduced_rows = sol.reduced_rows;
    res.reduced_cols = sol.reduced_cols;
    res.petrick_peak_terms = sol.petrick_peak_terms;

    std::vector<Cube> cover;
    for (std::size_t p : sol.selected) cover.push_back(chart.primes[p]);
    std::sort(cover.begin(), cover.end());
    res.cover = cover;

    for (std::size_t p : sol.essential) {
        auto it = std::find(res.cover.begin(), res.cover.end(), chart.primes[p]);
        res.essential.push_back(static_cast<std::size_t>(it - res.cover.begin()));
    }
    std::sort(res.essential.begin(), res.essential.end());
    for (std::size_t i = 0; i < res.cover.size(); ++i) res.selected.push_back(i);

    res.term_count = res.cover.size();
    for (const Cube& c : res.cover) res.literal_count += c.literal_count();
    return res;
}

bool verify_cover(const BoolFunction& f, const std::vector<Cube>& cover) {
    std::vector<Cube> on = f.on_minterms();
    std::vector<Cube> dc = f.dc_minterms();
    std::unordered_set<Cube, CubeHash> allowed(on.begin(), on.end());
    allowed.insert(dc.begin(), dc.end());

    for (const Cube& m : on) {
        bool hit = false;
        for (const Cube& c : cover)
            if (c.covers(m)) {
                hit = true;
                break;
            }
        if (!hit) return false;
    }
    for (const Cube& c : cover) {
        if (c.dash_count() > kMaxEnumVars) return false;
        for (const Cube& m : c.minterms())
            if (!allowed.count(m)) return false;
    }
    return true;
}

std::string format_sop(const std::vector<Cube>& cover, const std::vector<std::string>& names) {
    if (cover.empty()) return "0";
    std::string out;
    for (std::size_t i = 0; i < cover.size(); ++i) {
        if (i) out += " + ";
        out += cover[i].to_product(names);
    }
    return out;
}

std::string format_pos(const std::vector<Cube>& complement_cover, const std::vector<std::string>& names) {
    if (complement_cover.empty()) return "1";
    std::string out;
    for (std::size_t i = 0; i < complement_cover.size(); ++i) {
        out += complement_cover[i].to_sum(names);
    }
    return out;
}

} // namespace lss
