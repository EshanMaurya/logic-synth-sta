#include "lss/minimize.hpp"
#include "lss/parser.hpp"
#include "test_support.hpp"
#include <random>

using namespace lss;

namespace {

BoolFunction random_fn(std::mt19937& rng, std::size_t n, int p_on, int p_dc) {
    BoolFunction f;
    f.var_names = default_var_names(n);
    for (std::uint64_t m = 0; m < (1ULL << n); ++m) {
        int r = static_cast<int>(rng() % 100);
        if (r < p_on) f.on.push_back(Cube::from_minterm(n, m));
        else if (r < p_on + p_dc) f.dc.push_back(Cube::from_minterm(n, m));
    }
    return f;
}

}

int main() {
    {
        BoolFunction f = parse_minterm_list("f(a,b,c,d)=m(4,8,10,11,12,15)+d(9,14)", {});
        MinimizeResult e = minimize_espresso(f);
        CHECK(verify_cover(f, e.cover));
        CHECK(verify_cover_symbolic(f, e.cover));
        CHECK(e.term_count == minimize_exact(f).term_count);
    }
    {
        BoolFunction f;
        f.var_names = default_var_names(3);
        CHECK(minimize_espresso(f).cover.empty());
        f.on.push_back(Cube::universe(3));
        MinimizeResult r = minimize_espresso(f);
        CHECK(r.cover.size() == 1 && r.cover[0].literal_count() == 0);
    }
    std::mt19937 rng(12345);
    std::size_t total_e = 0, total_x = 0, worst_gap = 0;
    for (int t = 0; t < 300; ++t) {
        std::size_t n = 3 + rng() % 5;
        BoolFunction f = random_fn(rng, n, 20 + static_cast<int>(rng() % 50), static_cast<int>(rng() % 20));
        MinimizeResult e = minimize_espresso(f);
        CHECK(verify_cover(f, e.cover));
        CHECK(verify_cover_symbolic(f, e.cover));
        MinimizeResult x = minimize_exact(f);
        CHECK(e.term_count >= x.term_count || !x.exact);
        total_e += e.term_count;
        total_x += x.term_count;
        if (e.term_count > x.term_count) worst_gap = std::max(worst_gap, e.term_count - x.term_count);
    }
    std::cout << "espresso terms " << total_e << " vs exact " << total_x << " (worst gap " << worst_gap << ")\n";
    CHECK(total_e * 100 <= total_x * 112);
    {
        BoolFunction f;
        f.var_names = default_var_names(40);
        for (int k = 0; k < 30; ++k) {
            Cube c = Cube::universe(40);
            for (int j = 0; j < 6; ++j) c.set(rng() % 40, (rng() & 1) ? Lit::One : Lit::Zero);
            f.on.push_back(c);
        }
        MinimizeResult r = minimize_espresso(f);
        CHECK(verify_cover_symbolic(f, r.cover));
        CHECK(r.term_count <= f.on.size());
    }
    return TEST_SUMMARY("test_espresso");
}
