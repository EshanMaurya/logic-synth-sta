#include "lss/minimize.hpp"
#include <algorithm>
#include <stdexcept>

namespace lss {

PrimeImplicantChart PrimeImplicantChart::build(const std::vector<Cube>& primes, const std::vector<Cube>& on_minterms) {
    PrimeImplicantChart chart;
    chart.minterms = on_minterms;
    const std::size_t m = on_minterms.size();

    for (const Cube& p : primes) {
        BitVec cov(m);
        for (std::size_t i = 0; i < m; ++i)
            if (p.covers(on_minterms[i])) cov.set(i);
        if (cov.none()) continue;
        chart.primes.push_back(p);
        chart.covers.push_back(std::move(cov));
    }

    const std::size_t np = chart.primes.size();
    chart.covered_by.assign(m, BitVec(np));
    for (std::size_t p = 0; p < np; ++p)
        for (std::size_t i = chart.covers[p].find_first(); i != BitVec::npos; i = chart.covers[p].find_next(i))
            chart.covered_by[i].set(p);

    return chart;
}

namespace {

constexpr std::size_t kDominanceLimit = 4096;
constexpr std::size_t kBranchNodeLimit = 400000;

struct CostKey {
    std::size_t terms;
    std::size_t literals;
    bool operator<(const CostKey& o) const {
        if (terms != o.terms) return terms < o.terms;
        return literals < o.literals;
    }
};

std::size_t literals_of(const PrimeImplicantChart& chart, const BitVec& set) {
    std::size_t total = 0;
    for (std::size_t p = set.find_first(); p != BitVec::npos; p = set.find_next(p))
        total += chart.primes[p].literal_count();
    return total;
}

void absorb(std::vector<BitVec>& terms) {
    std::sort(terms.begin(), terms.end(), [](const BitVec& a, const BitVec& b) {
        std::size_t ca = a.count();
        std::size_t cb = b.count();
        if (ca != cb) return ca < cb;
        return a < b;
    });
    terms.erase(std::unique(terms.begin(), terms.end()), terms.end());

    std::vector<BitVec> kept;
    kept.reserve(terms.size());
    for (BitVec& t : terms) {
        bool dominated = false;
        for (const BitVec& k : kept) {
            if (k.subset_of(t)) {
                dominated = true;
                break;
            }
        }
        if (!dominated) kept.push_back(std::move(t));
    }
    terms = std::move(kept);
}

bool petrick(const PrimeImplicantChart& chart, const BitVec& active_primes, const BitVec& rows,
             std::size_t limit, BitVec& chosen, std::size_t& peak) {
    std::vector<std::size_t> compact;
    std::vector<std::size_t> to_compact(chart.primes.size(), BitVec::npos);
    for (std::size_t p = active_primes.find_first(); p != BitVec::npos; p = active_primes.find_next(p)) {
        to_compact[p] = compact.size();
        compact.push_back(p);
    }
    const std::size_t width = compact.size();

    std::vector<BitVec> clauses;
    for (std::size_t r = rows.find_first(); r != BitVec::npos; r = rows.find_next(r)) {
        BitVec clause(width);
        BitVec cand = chart.covered_by[r];
        cand &= active_primes;
        for (std::size_t p = cand.find_first(); p != BitVec::npos; p = cand.find_next(p))
            clause.set(to_compact[p]);
        clauses.push_back(std::move(clause));
    }
    absorb(clauses);
    std::sort(clauses.begin(), clauses.end(), [](const BitVec& a, const BitVec& b) {
        return a.count() < b.count();
    });

    std::vector<BitVec> terms;
    terms.emplace_back(width);

    for (const BitVec& clause : clauses) {
        std::vector<BitVec> next;
        for (const BitVec& t : terms) {
            if (t.intersects(clause)) {
                next.push_back(t);
                continue;
            }
            for (std::size_t p = clause.find_first(); p != BitVec::npos; p = clause.find_next(p)) {
                BitVec nt = t;
                nt.set(p);
                next.push_back(std::move(nt));
            }
        }
        absorb(next);
        terms = std::move(next);
        peak = std::max(peak, terms.size());
        if (terms.size() > limit) return false;
    }

    CostKey best_key{static_cast<std::size_t>(-1), static_cast<std::size_t>(-1)};
    const BitVec* best = nullptr;
    for (const BitVec& t : terms) {
        BitVec full(chart.primes.size());
        for (std::size_t c = t.find_first(); c != BitVec::npos; c = t.find_next(c)) full.set(compact[c]);
        CostKey key{full.count(), literals_of(chart, full)};
        if (best == nullptr || key < best_key) {
            best_key = key;
            best = &t;
        }
    }

    chosen = BitVec(chart.primes.size());
    if (best != nullptr)
        for (std::size_t c = best->find_first(); c != BitVec::npos; c = best->find_next(c)) chosen.set(compact[c]);
    return true;
}

void greedy(const PrimeImplicantChart& chart, BitVec active, BitVec rows, BitVec& chosen) {
    chosen = BitVec(chart.primes.size());
    while (rows.any()) {
        std::size_t best_p = BitVec::npos;
        std::size_t best_gain = 0;
        for (std::size_t p = active.find_first(); p != BitVec::npos; p = active.find_next(p)) {
            BitVec g = chart.covers[p];
            g &= rows;
            std::size_t gain = g.count();
            if (gain == 0) continue;
            bool better = best_p == BitVec::npos || gain > best_gain ||
                          (gain == best_gain &&
                           chart.primes[p].literal_count() < chart.primes[best_p].literal_count());
            if (better) {
                best_p = p;
                best_gain = gain;
            }
        }
        if (best_p == BitVec::npos) throw std::logic_error("uncoverable minterm in chart");
        chosen.set(best_p);
        rows.subtract(chart.covers[best_p]);
        active.reset(best_p);
    }
}

struct BranchAndBound {
    const PrimeImplicantChart& chart;
    std::size_t node_limit;
    std::size_t nodes = 0;
    bool aborted = false;
    CostKey best{static_cast<std::size_t>(-1), static_cast<std::size_t>(-1)};
    BitVec best_set;

    BranchAndBound(const PrimeImplicantChart& c, std::size_t limit) : chart(c), node_limit(limit), best_set(c.primes.size()) {}

    void run(BitVec active, const BitVec& rows, const BitVec& cur, std::size_t terms, std::size_t lits) {
        if (aborted) return;
        if (rows.none()) {
            CostKey key{terms, lits};
            if (key < best) {
                best = key;
                best_set = cur;
            }
            return;
        }
        if (++nodes > node_limit) {
            aborted = true;
            return;
        }

        std::size_t row_count = rows.count();
        std::size_t max_gain = 0;
        for (std::size_t p = active.find_first(); p != BitVec::npos; p = active.find_next(p)) {
            BitVec g = chart.covers[p];
            g &= rows;
            max_gain = std::max(max_gain, g.count());
        }
        if (max_gain == 0) return;
        std::size_t lb = (row_count + max_gain - 1) / max_gain;
        if (terms + lb > best.terms) return;
        if (terms + lb == best.terms && lits >= best.literals) return;

        std::size_t pivot = BitVec::npos;
        std::size_t pivot_count = static_cast<std::size_t>(-1);
        for (std::size_t r = rows.find_first(); r != BitVec::npos; r = rows.find_next(r)) {
            BitVec cand = chart.covered_by[r];
            cand &= active;
            std::size_t c = cand.count();
            if (c == 0) return;
            if (c < pivot_count) {
                pivot_count = c;
                pivot = r;
            }
        }

        BitVec cand = chart.covered_by[pivot];
        cand &= active;
        std::vector<std::size_t> order;
        for (std::size_t p = cand.find_first(); p != BitVec::npos; p = cand.find_next(p)) order.push_back(p);
        std::vector<std::size_t> gains(chart.primes.size(), 0);
        for (std::size_t p : order) {
            BitVec g = chart.covers[p];
            g &= rows;
            gains[p] = g.count();
        }
        std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
            if (gains[a] != gains[b]) return gains[a] > gains[b];
            std::size_t la = chart.primes[a].literal_count();
            std::size_t lb2 = chart.primes[b].literal_count();
            if (la != lb2) return la < lb2;
            return a < b;
        });

        for (std::size_t p : order) {
            BitVec next_rows = rows;
            next_rows.subtract(chart.covers[p]);
            BitVec next_cur = cur;
            next_cur.set(p);
            run(active, next_rows, next_cur, terms + 1, lits + chart.primes[p].literal_count());
            if (aborted) return;
            active.reset(p);
        }
    }
};

bool exact_search(const PrimeImplicantChart& chart, const BitVec& active, const BitVec& rows,
                  std::size_t node_limit, BitVec& chosen) {
    BitVec seed;
    greedy(chart, active, rows, seed);

    BranchAndBound bb(chart, node_limit);
    bb.best = CostKey{seed.count(), literals_of(chart, seed)};
    bb.best_set = seed;
    bb.run(active, rows, BitVec(chart.primes.size()), 0, 0);
    chosen = bb.best_set;
    return !bb.aborted;
}

} // namespace

CoverSolution solve_cover(const PrimeImplicantChart& chart, std::size_t petrick_term_limit) {
    const std::size_t np = chart.primes.size();
    const std::size_t nm = chart.minterms.size();

    CoverSolution sol;
    BitVec active(np);
    for (std::size_t p = 0; p < np; ++p) active.set(p);
    BitVec rows(nm);
    for (std::size_t r = 0; r < nm; ++r) rows.set(r);
    BitVec forced(np);

    bool first_pass = true;
    bool changed = true;
    while (changed && rows.any()) {
        changed = false;

        bool essential_found = true;
        while (essential_found && rows.any()) {
            essential_found = false;
            for (std::size_t r = rows.find_first(); r != BitVec::npos; r = rows.find_next(r)) {
                BitVec cand = chart.covered_by[r];
                cand &= active;
                std::size_t c = cand.count();
                if (c == 0) throw std::logic_error("minterm with no covering prime implicant");
                if (c != 1) continue;
                std::size_t p = cand.find_first();
                forced.set(p);
                if (first_pass) sol.essential.push_back(p);
                rows.subtract(chart.covers[p]);
                active.reset(p);
                essential_found = true;
                changed = true;
                break;
            }
        }
        first_pass = false;
        if (rows.none()) break;

        for (std::size_t p = active.find_first(); p != BitVec::npos; p = active.find_next(p)) {
            BitVec g = chart.covers[p];
            g &= rows;
            if (g.none()) active.reset(p);
        }

        std::vector<std::size_t> row_ids;
        for (std::size_t r = rows.find_first(); r != BitVec::npos; r = rows.find_next(r)) row_ids.push_back(r);
        if (row_ids.size() <= kDominanceLimit) {
            std::vector<BitVec> row_sets;
            row_sets.reserve(row_ids.size());
            for (std::size_t r : row_ids) {
                BitVec s = chart.covered_by[r];
                s &= active;
                row_sets.push_back(std::move(s));
            }
            std::vector<bool> drop(row_ids.size(), false);
            for (std::size_t i = 0; i < row_ids.size(); ++i) {
                if (drop[i]) continue;
                for (std::size_t j = 0; j < row_ids.size(); ++j) {
                    if (i == j || drop[j]) continue;
                    if (row_sets[i].subset_of(row_sets[j])) {
                        bool equal = row_sets[i] == row_sets[j];
                        if (!equal || i < j) {
                            drop[j] = true;
                            rows.reset(row_ids[j]);
                            ++sol.reduced_rows;
                            changed = true;
                        }
                    }
                }
            }
        }

        std::vector<std::size_t> col_ids;
        for (std::size_t p = active.find_first(); p != BitVec::npos; p = active.find_next(p)) col_ids.push_back(p);
        if (col_ids.size() <= kDominanceLimit) {
            std::vector<BitVec> col_sets;
            col_sets.reserve(col_ids.size());
            for (std::size_t p : col_ids) {
                BitVec s = chart.covers[p];
                s &= rows;
                col_sets.push_back(std::move(s));
            }
            std::vector<bool> drop(col_ids.size(), false);
            for (std::size_t j = 0; j < col_ids.size(); ++j) {
                if (drop[j]) continue;
                for (std::size_t k = 0; k < col_ids.size(); ++k) {
                    if (j == k || drop[k]) continue;
                    if (!col_sets[j].subset_of(col_sets[k])) continue;
                    std::size_t lj = chart.primes[col_ids[j]].literal_count();
                    std::size_t lk = chart.primes[col_ids[k]].literal_count();
                    if (lk > lj) continue;
                    bool same = col_sets[j] == col_sets[k] && lj == lk;
                    if (same && k > j) continue;
                    drop[j] = true;
                    active.reset(col_ids[j]);
                    ++sol.reduced_cols;
                    changed = true;
                    break;
                }
            }
        }
    }

    BitVec chosen(np);
    if (rows.any()) {
        if (!petrick(chart, active, rows, petrick_term_limit, chosen, sol.petrick_peak_terms)) {
            sol.exact = exact_search(chart, active, rows, kBranchNodeLimit, chosen);
        }
    }

    BitVec all = forced;
    all |= chosen;
    for (std::size_t p = all.find_first(); p != BitVec::npos; p = all.find_next(p)) sol.selected.push_back(p);
    std::sort(sol.essential.begin(), sol.essential.end());
    return sol;
}

} // namespace lss
