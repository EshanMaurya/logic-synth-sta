#include "lss/minimize.hpp"
#include "lss/parser.hpp"
#include "test_support.hpp"
#include <algorithm>
#include <random>
#include <set>

using namespace lss;

namespace {

std::set<std::string> strs(const std::vector<Cube>& v) {
    std::set<std::string> s;
    for (const Cube& c : v) s.insert(c.to_string());
    return s;
}

struct Cost {
    std::size_t terms;
    std::size_t literals;
    bool operator==(const Cost& o) const { return terms == o.terms && literals == o.literals; }
    bool operator<(const Cost& o) const { return terms != o.terms ? terms < o.terms : literals < o.literals; }
};

Cost brute_force_cost(const BoolFunction& f) {
    std::vector<Cube> on = f.on_minterms();
    if (on.empty()) return {0, 0};
    std::vector<Cube> all = on;
    for (const Cube& c : f.dc_minterms()) all.push_back(c);
    std::vector<Cube> primes = generate_primes(all);
    std::vector<Cube> useful;
    for (const Cube& p : primes)
        for (const Cube& m : on)
            if (p.covers(m)) {
                useful.push_back(p);
                break;
            }
    const std::size_t np = useful.size();
    Cost best{static_cast<std::size_t>(-1), static_cast<std::size_t>(-1)};
    for (std::uint64_t mask = 1; mask < (1ULL << np); ++mask) {
        Cost c{0, 0};
        for (std::size_t i = 0; i < np; ++i)
            if ((mask >> i) & 1ULL) {
                ++c.terms;
                c.literals += useful[i].literal_count();
            }
        if (!(c < best)) continue;
        bool ok = true;
        for (const Cube& m : on) {
            bool hit = false;
            for (std::size_t i = 0; i < np && !hit; ++i)
                if (((mask >> i) & 1ULL) && useful[i].covers(m)) hit = true;
            if (!hit) {
                ok = false;
                break;
            }
        }
        if (ok) best = c;
    }
    return best;
}

std::set<std::string> brute_primes(const BoolFunction& f) {
    std::size_t n = f.num_vars();
    std::set<std::string> allowed;
    for (const Cube& c : f.on_minterms()) allowed.insert(c.to_string());
    for (const Cube& c : f.dc_minterms()) allowed.insert(c.to_string());

    std::vector<Cube> implicants;
    std::size_t total = 1;
    for (std::size_t i = 0; i < n; ++i) total *= 3;
    for (std::size_t code = 0; code < total; ++code) {
        Cube c(n);
        std::size_t x = code;
        for (std::size_t v = 0; v < n; ++v) {
            c.set(v, static_cast<Lit>(x % 3));
            x /= 3;
        }
        bool inside = true;
        for (const Cube& m : c.minterms())
            if (!allowed.count(m.to_string())) {
                inside = false;
                break;
            }
        if (inside) implicants.push_back(c);
    }
    std::set<std::string> primes;
    for (const Cube& c : implicants) {
        bool maximal = true;
        for (const Cube& d : implicants)
            if (d != c && d.covers(c)) {
                maximal = false;
                break;
            }
        if (maximal) primes.insert(c.to_string());
    }
    return primes;
}

BoolFunction random_function(std::mt19937& rng, std::size_t n, int on_pct, int dc_pct) {
    BoolFunction f;
    f.var_names = default_var_names(n);
    std::uniform_int_distribution<int> d(0, 99);
    for (std::uint64_t a = 0; a < (1ULL << n); ++a) {
        int r = d(rng);
        if (r < on_pct) f.on.push_back(Cube::from_minterm(n, a));
        else if (r < on_pct + dc_pct) f.dc.push_back(Cube::from_minterm(n, a));
    }
    return f;
}

} // namespace

int main() {
    {
        BoolFunction f = parse_minterm_list("f(a,b,c,d) = m(4,8,10,11,12,15) + d(9,14)");
        auto primes = generate_primes([&] {
            auto v = f.on_minterms();
            for (const Cube& c : f.dc_minterms()) v.push_back(c);
            return v;
        }());
        CHECK(strs(primes) == (std::set<std::string>{"-100", "1-0-", "10--", "1--0", "1-1-"}) ||
              strs(primes) == brute_primes(f));
        MinimizeResult r = minimize_exact(f);
        CHECK(verify_cover(f, r.cover));
        CHECK(r.term_count == 3);
        CHECK(r.exact);
    }

    {
        BoolFunction f = parse_minterm_list("f(a,b,c,d) = m(0,1,2,5,6,7,8,9,10,14)");
        MinimizeResult r = minimize_exact(f);
        CHECK(verify_cover(f, r.cover));
        CHECK(strs(r.primes) == brute_primes(f));
        Cost bf = brute_force_cost(f);
        CHECK((Cost{r.term_count, r.literal_count} == bf));
    }

    {
        BoolFunction f = parse_minterm_list("f(a,b,c) = m(0,1,2,5,6,7)");
        MinimizeResult r = minimize_exact(f);
        CHECK(r.essential.empty());
        CHECK(r.term_count == 3);
        CHECK(r.literal_count == 6);
        CHECK(r.primes.size() == 6);
        CHECK(verify_cover(f, r.cover));
    }

    {
        BoolFunction f = parse_minterm_list("f(a,b,c,d) = m(0,2,5,6,7,8,10,12,13,14,15)");
        MinimizeResult r = minimize_exact(f);
        CHECK(verify_cover(f, r.cover));
        CHECK((Cost{r.term_count, r.literal_count} == brute_force_cost(f)));
    }

    {
        BoolFunction f = parse_expression("ab + a'b + ab'");
        MinimizeResult r = minimize_exact(f);
        CHECK(strs(r.cover) == (std::set<std::string>{"-1", "1-"}) || strs(r.cover) == (std::set<std::string>{"1-", "-1"}));
        CHECK(r.literal_count == 2);
    }

    {
        BoolFunction zero;
        zero.var_names = default_var_names(3);
        MinimizeResult r = minimize_exact(zero);
        CHECK(r.cover.empty());
        CHECK(format_sop(r.cover, zero.var_names) == "0");

        BoolFunction one = parse_truth_string("11111111");
        MinimizeResult r1 = minimize_exact(one);
        CHECK(r1.cover.size() == 1);
        CHECK(r1.cover[0].to_string() == "---");
        CHECK(format_sop(r1.cover, one.var_names) == "1");
    }

    {
        BoolFunction f = parse_truth_string("0000 0000 11xx xxxx");
        MinimizeResult r = minimize_exact(f);
        CHECK(r.term_count == 1);
        CHECK(r.literal_count == 1);
        CHECK(verify_cover(f, r.cover));
    }

    {
        BoolFunction f = parse_expression("a ^ b ^ c ^ d");
        MinimizeResult r = minimize_exact(f);
        CHECK(r.term_count == 8);
        CHECK(r.literal_count == 32);
        CHECK(r.essential.size() == 8);
    }

    {
        BoolFunction f = parse_expression("a'b'c' + a'bc + ab'c + abc'");
        BoolFunction c = f.complement();
        MinimizeResult r = minimize_exact(c);
        CHECK(verify_cover(c, r.cover));
        CHECK(format_pos(r.cover, f.var_names).find('(') != std::string::npos);
    }

    {
        std::mt19937 rng(20260916);
        int exact_checked = 0;
        for (int iter = 0; iter < 300; ++iter) {
            std::size_t n = 2 + static_cast<std::size_t>(iter % 5);
            int on_pct = 15 + (iter * 7) % 55;
            int dc_pct = (iter % 3 == 0) ? 15 : 0;
            BoolFunction f = random_function(rng, n, on_pct, dc_pct);
            MinimizeResult r = minimize_exact(f);
            CHECK(r.exact);
            CHECK(verify_cover(f, r.cover));
            std::vector<Cube> on = f.on_minterms();
            if (n <= 4 && !on.empty()) CHECK(strs(r.primes) == brute_primes(f));
            if (on.empty()) {
                CHECK(r.cover.empty());
                continue;
            }
            std::vector<Cube> all = on;
            for (const Cube& d : f.dc_minterms()) all.push_back(d);
            std::vector<Cube> primes = generate_primes(all);
            std::size_t useful = 0;
            for (const Cube& p : primes)
                for (const Cube& m : on)
                    if (p.covers(m)) {
                        ++useful;
                        break;
                    }
            if (useful <= 12) {
                CHECK((Cost{r.term_count, r.literal_count} == brute_force_cost(f)));
                ++exact_checked;
            }
        }
        CHECK(exact_checked > 150);
    }

    {
        std::mt19937 rng(77);
        for (int iter = 0; iter < 6; ++iter) {
            BoolFunction f = random_function(rng, 8, 40, 10);
            MinimizeResult r = minimize_exact(f);
            CHECK(verify_cover(f, r.cover));
            MinimizeResult c = minimize_exact(f.complement());
            CHECK(verify_cover(f.complement(), c.cover));
        }
    }

    {
        BoolFunction f = parse_expression("a1 a2' a3 + a4 a5 + a6'a7 + a8 a1'", "", {});
        MinimizeResult r = minimize_exact(f);
        CHECK(verify_cover(f, r.cover));
        CHECK(r.term_count == 4);
    }

    {
        BoolFunction f = parse_expression("a b c d e f g h i j k l m n o p q r s t");
        CHECK(f.num_vars() == 20);
        CHECK(f.on_minterms().size() == 1);
        MinimizeResult r = minimize_exact(f);
        CHECK(r.term_count == 1);
        CHECK(r.literal_count == 20);
    }

    return TEST_SUMMARY("test_minimization");
}
