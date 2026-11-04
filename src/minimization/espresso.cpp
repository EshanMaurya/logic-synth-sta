#include "lss/minimize.hpp"
#include <algorithm>
#include <numeric>
#include <unordered_set>

namespace lss {

namespace {

using Cover = std::vector<Cube>;

struct Cost {
    std::size_t terms = 0, lits = 0;
    bool operator<(const Cost& o) const { return terms != o.terms ? terms < o.terms : lits < o.lits; }
};

Cost cost_of(const Cover& f) {
    Cost c;
    c.terms = f.size();
    for (const Cube& q : f) c.lits += q.literal_count();
    return c;
}
Cover scc(Cover f) {
    std::sort(f.begin(), f.end(), [](const Cube& a, const Cube& b) {
        if (a.dash_count() != b.dash_count()) return a.dash_count() > b.dash_count();
        return a < b;
    });
    Cover out;
    for (const Cube& c : f) {
        bool covered = false;
        for (const Cube& k : out)
            if (k.covers(c)) {
                covered = true;
                break;
            }
        if (!covered) out.push_back(c);
    }
    return out;
}

bool is_universe(const Cube& c) { return c.literal_count() == 0; }
Cover cofactor(const Cover& f, const Cube& p) {
    std::vector<std::size_t> care;
    for (std::size_t i = 0; i < p.num_vars(); ++i)
        if (p.get(i) != Lit::DontCare) care.push_back(i);
    Cover out;
    for (const Cube& c : f) {
        if (!c.intersects(p)) continue;
        Cube r = c;
        for (std::size_t v : care) r.set(v, Lit::DontCare);
        out.push_back(std::move(r));
    }
    return out;
}

Cover cofactor_var(const Cover& f, std::size_t v, Lit l) {
    Cover out;
    for (const Cube& c : f) {
        Lit cl = c.get(v);
        if (cl != Lit::DontCare && cl != l) continue;
        Cube r = c;
        r.set(v, Lit::DontCare);
        out.push_back(std::move(r));
    }
    return out;
}
std::size_t pick_var(const Cover& f, std::size_t n, bool& binate) {
    std::size_t best = n, best_score = 0;
    binate = false;
    for (std::size_t v = 0; v < n; ++v) {
        std::size_t z = 0, o = 0;
        for (const Cube& c : f) {
            Lit l = c.get(v);
            if (l == Lit::Zero) ++z;
            else if (l == Lit::One) ++o;
        }
        if (z + o == 0) continue;
        bool b = z && o;
        std::size_t score = (b ? (1ULL << 40) : 0) + (z + o) * 1024 + (1023 - std::min<std::size_t>(1023, z > o ? z - o : o - z));
        if (b) binate = true;
        if (score > best_score) {
            best_score = score;
            best = v;
        }
    }
    return best;
}

bool tautology(const Cover& f, std::size_t n) {
    if (f.empty()) return false;
    for (const Cube& c : f)
        if (is_universe(c)) return true;
    bool binate;
    std::size_t v = pick_var(f, n, binate);
    if (v == n || !binate) return false;
    return tautology(cofactor_var(f, v, Lit::Zero), n) && tautology(cofactor_var(f, v, Lit::One), n);
}

bool covered_by(const Cover& f, const Cube& c) {
    if (f.empty()) return false;
    return tautology(cofactor(f, c), c.num_vars());
}

Cover complement(const Cover& f, std::size_t n) {
    if (f.empty()) return {Cube::universe(n)};
    for (const Cube& c : f)
        if (is_universe(c)) return {};
    if (f.size() == 1) {
        Cover out;
        for (std::size_t v = 0; v < n; ++v) {
            Lit l = f[0].get(v);
            if (l == Lit::DontCare) continue;
            Cube r = Cube::universe(n);
            r.set(v, l == Lit::One ? Lit::Zero : Lit::One);
            out.push_back(std::move(r));
        }
        return out;
    }
    bool binate;
    std::size_t v = pick_var(f, n, binate);
    Cover c0 = complement(cofactor_var(f, v, Lit::Zero), n);
    Cover c1 = complement(cofactor_var(f, v, Lit::One), n);
    std::unordered_set<Cube, CubeHash> in1(c1.begin(), c1.end());
    std::unordered_set<Cube, CubeHash> shared;
    Cover out;
    for (Cube& c : c0) {
        if (in1.count(c)) {
            shared.insert(c);
            out.push_back(c);
        } else {
            c.set(v, Lit::Zero);
            out.push_back(std::move(c));
        }
    }
    for (Cube& c : c1) {
        if (shared.count(c)) continue;
        c.set(v, Lit::One);
        out.push_back(std::move(c));
    }
    return out.size() <= 3000 ? scc(std::move(out)) : out;
}

bool hits_off(const Cube& c, const Cover& off) {
    for (const Cube& o : off)
        if (c.intersects(o)) return true;
    return false;
}
Cover expand(Cover f, const Cover& off, std::size_t n) {
    std::stable_sort(f.begin(), f.end(), [](const Cube& a, const Cube& b) { return a.dash_count() < b.dash_count(); });
    std::vector<bool> dead(f.size(), false);
    Cover out;
    for (std::size_t i = 0; i < f.size(); ++i) {
        if (dead[i]) continue;
        Cube c = f[i];
        std::vector<std::pair<std::size_t, std::size_t>> order;
        for (std::size_t v = 0; v < n; ++v) {
            Lit l = c.get(v);
            if (l == Lit::DontCare) continue;
            std::size_t agree = 0;
            for (std::size_t j = 0; j < f.size(); ++j)
                if (j != i && !dead[j] && f[j].get(v) == l) ++agree;
            order.emplace_back(agree, v);
        }
        std::sort(order.begin(), order.end());
        for (auto& entry : order) {
            std::size_t v = entry.second;
            Cube t = c;
            t.set(v, Lit::DontCare);
            if (!hits_off(t, off)) c = std::move(t);
        }
        for (std::size_t j = i + 1; j < f.size(); ++j)
            if (!dead[j] && c.covers(f[j])) dead[j] = true;
        out.push_back(std::move(c));
    }
    return scc(std::move(out));
}
Cover irredundant(Cover f, const Cover& dc) {
    std::stable_sort(f.begin(), f.end(), [](const Cube& a, const Cube& b) { return a.dash_count() < b.dash_count(); });
    std::vector<bool> keep(f.size(), true);
    for (std::size_t i = 0; i < f.size(); ++i) {
        Cover rest = dc;
        for (std::size_t j = 0; j < f.size(); ++j)
            if (j != i && keep[j]) rest.push_back(f[j]);
        if (covered_by(rest, f[i])) keep[i] = false;
    }
    Cover out;
    for (std::size_t i = 0; i < f.size(); ++i)
        if (keep[i]) out.push_back(f[i]);
    return out;
}
Cover reduce(Cover f, const Cover& dc, std::size_t n) {
    std::stable_sort(f.begin(), f.end(), [](const Cube& a, const Cube& b) { return a.dash_count() > b.dash_count(); });
    for (std::size_t i = 0; i < f.size(); ++i) {
        Cover rest = dc;
        for (std::size_t j = 0; j < f.size(); ++j)
            if (j != i) rest.push_back(f[j]);
        Cover comp = complement(cofactor(rest, f[i]), n);
        if (comp.empty()) {
            f.erase(f.begin() + static_cast<std::ptrdiff_t>(i));
            --i;
            continue;
        }
        Cube sc = comp[0];
        for (std::size_t k = 1; k < comp.size(); ++k)
            for (std::size_t v = 0; v < n; ++v)
                if (sc.get(v) != comp[k].get(v)) sc.set(v, Lit::DontCare);
        Cube r = f[i];
        for (std::size_t v = 0; v < n; ++v)
            if (r.get(v) == Lit::DontCare) r.set(v, sc.get(v));
        f[i] = std::move(r);
    }
    return f;
}

}

MinimizeResult minimize_espresso(const BoolFunction& f, const EspressoOptions& opts) {
    MinimizeResult res;
    res.exact = false;
    const std::size_t n = f.num_vars();
    if (f.on.empty()) return res;

    Cover on = scc(f.on);
    Cover dc = f.dc;
    Cover care = on;
    care.insert(care.end(), dc.begin(), dc.end());
    Cover off = complement(care, n);

    Cover cur = expand(on, off, n);
    cur = irredundant(std::move(cur), dc);
    for (std::size_t it = 0; it < opts.max_iterations; ++it) {
        Cover g = reduce(cur, dc, n);
        g = expand(std::move(g), off, n);
        g = irredundant(std::move(g), dc);
        if (cost_of(g) < cost_of(cur)) cur = std::move(g);
        else break;
    }
    std::sort(cur.begin(), cur.end());
    res.cover = cur;
    for (std::size_t i = 0; i < cur.size(); ++i) {
        Cover rest = dc;
        for (std::size_t j = 0; j < cur.size(); ++j)
            if (j != i) rest.push_back(cur[j]);
        if (!covered_by(rest, cur[i])) res.essential.push_back(i);
    }
    for (std::size_t i = 0; i < cur.size(); ++i) res.selected.push_back(i);
    Cost c = cost_of(cur);
    res.term_count = c.terms;
    res.literal_count = c.lits;
    return res;
}

bool verify_cover_symbolic(const BoolFunction& f, const std::vector<Cube>& cover) {
    const std::size_t n = f.num_vars();
    Cover care = f.on;
    care.insert(care.end(), f.dc.begin(), f.dc.end());
    Cover off = complement(care, n);
    for (const Cube& c : cover)
        if (hits_off(c, off)) return false;
    Cover allowed = cover;
    allowed.insert(allowed.end(), f.dc.begin(), f.dc.end());
    for (const Cube& o : f.on)
        if (!covered_by(allowed, o)) return false;
    return true;
}

}
