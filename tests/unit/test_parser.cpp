#include "lss/parser.hpp"
#include "test_support.hpp"
#include <algorithm>
#include <set>

using namespace lss;

namespace {

std::set<std::string> on_strings(const BoolFunction& f) {
    std::set<std::string> s;
    for (const Cube& c : f.on_minterms()) s.insert(c.to_string());
    return s;
}

std::set<std::string> dc_strings(const BoolFunction& f) {
    std::set<std::string> s;
    for (const Cube& c : f.dc_minterms()) s.insert(c.to_string());
    return s;
}

bool throws_parse_error(const std::function<void()>& fn) {
    try {
        fn();
    } catch (const ParseError&) {
        return true;
    }
    return false;
}

} // namespace

int main() {
    {
        BoolFunction f = parse_expression("a'b + cd");
        CHECK(f.var_names == (std::vector<std::string>{"a", "b", "c", "d"}));
        CHECK(f.on_minterms().size() == 4 + 4 - 1);
        CHECK(on_strings(f).count("0100"));
        CHECK(on_strings(f).count("1111"));
        CHECK(!on_strings(f).count("1000"));
    }
    {
        BoolFunction sop = parse_expression("ab' + a'b");
        BoolFunction x = parse_expression("a ^ b");
        CHECK(on_strings(sop) == on_strings(x));
    }
    {
        BoolFunction pos = parse_expression("(a+b')(c+d)");
        CHECK(pos.on_minterms().size() == 9);
        BoolFunction sop = parse_expression("ac + ad + b'c + b'd");
        CHECK(on_strings(pos) == on_strings(sop));
    }
    {
        BoolFunction f = parse_expression("!a & (b | ~c)");
        CHECK(f.on_minterms().size() == 3);
    }
    {
        BoolFunction f = parse_expression("a*b + c", "a'b'c'");
        CHECK(dc_strings(f).count("000"));
        CHECK(f.dc_minterms().size() == 1);
    }
    {
        BoolFunction f = parse_expression("x1 x2' + x10", "", {});
        CHECK(f.var_names == (std::vector<std::string>{"x1", "x2", "x10"}));
    }
    {
        BoolFunction f = parse_expression("req*grant' + done", "", {"req", "grant", "done"});
        CHECK(f.num_vars() == 3);
        CHECK(f.on_minterms().size() == 2 + 4 - 1);
    }
    {
        BoolFunction f = parse_expression("a b' c", "", {"c", "b", "a"});
        CHECK(on_strings(f) == std::set<std::string>{"101"});
    }
    CHECK(throws_parse_error([] { parse_expression("a + "); }));
    CHECK(throws_parse_error([] { parse_expression("(a + b"); }));
    CHECK(throws_parse_error([] { parse_expression("a # b"); }));
    CHECK(throws_parse_error([] { parse_expression("ab", "", {"a"}); }));

    {
        BoolFunction f = parse_minterm_list("f(a,b,c,d) = m(0,1,2,5,8-10) + d(6,7)");
        CHECK(f.name == "f");
        CHECK(f.num_vars() == 4);
        CHECK(f.on_minterms().size() == 7);
        CHECK(f.dc_minterms().size() == 2);
        CHECK(on_strings(f).count("1010"));
    }
    {
        BoolFunction f = parse_minterm_list("m(1,3,7)");
        CHECK(f.num_vars() == 3);
        CHECK(f.on_minterms().size() == 3);
    }
    {
        BoolFunction f = parse_minterm_list("g(x,y,z) = M(0,1,2,3) + d(4)");
        CHECK(f.on_minterms().size() == 3);
        CHECK(f.dc_minterms().size() == 1);
        CHECK(f.name == "g");
    }
    {
        BoolFunction f = parse_minterm_list("m(1,5)", {"p", "q", "r"});
        CHECK(f.var_names[0] == "p");
        CHECK(f.num_vars() == 3);
    }
    CHECK(throws_parse_error([] { parse_minterm_list("f(a,b) = m(0,9)"); }));
    CHECK(throws_parse_error([] { parse_minterm_list("f(a,b) = m(0) + M(1)"); }));
    CHECK(throws_parse_error([] { parse_minterm_list("f(a,b) = q(0)"); }));

    {
        BoolFunction f = parse_truth_string("0110 100x");
        CHECK(f.num_vars() == 3);
        CHECK(on_strings(f) == (std::set<std::string>{"001", "010", "100"}));
        CHECK(dc_strings(f) == (std::set<std::string>{"111"}));
    }
    CHECK(throws_parse_error([] { parse_truth_string("011"); }));
    CHECK(throws_parse_error([] { parse_truth_string("01q1"); }));

    {
        auto fs = parse_pla_string(
            "# comment\n.i 3\n.o 2\n.ilb a b c\n.ob s co\n.p 4\n"
            "001 10\n010 10\n100 10\n111 11\n011 01\n101 01\n110 01\n.e\n");
        CHECK(fs.size() == 2);
        CHECK(fs[0].name == "s");
        CHECK(fs[1].name == "co");
        CHECK(fs[0].on_minterms().size() == 4);
        CHECK(fs[1].on_minterms().size() == 4);
        CHECK(fs[0].var_names == (std::vector<std::string>{"a", "b", "c"}));
    }
    {
        auto fs = parse_pla_string(".i 4\n.o 1\n0-1- 1\n1--0 -\n.e\n");
        CHECK(fs.size() == 1);
        CHECK(fs[0].on_minterms().size() == 4);
        CHECK(fs[0].dc_minterms().size() == 4);
    }
    {
        auto fs = parse_pla_string(".i 2\n.o 1\n.type fr\n11 1\n00 0\n.e\n");
        CHECK(fs[0].on_minterms().size() == 1);
        CHECK(fs[0].dc_minterms().size() == 2);
    }
    CHECK(throws_parse_error([] { parse_pla_string(".i 3\n.o 1\n01 1\n"); }));
    CHECK(throws_parse_error([] { parse_pla_string(".i 2\n.o 1\n0q 1\n"); }));

    {
        Cube a = Cube::from_string("01-1");
        Cube b = Cube::from_string("0111");
        Cube c = Cube::from_string("1---");
        CHECK(a.covers(b));
        CHECK(!b.covers(a));
        CHECK(!a.intersects(c));
        CHECK(a.distance(c) == 1);
        CHECK(a.literal_count() == 3);
        CHECK(a.minterms().size() == 2);
        CHECK(Cube::from_string("0101").merge_adjacent(Cube::from_string("0111"))->to_string() == "01-1");
        CHECK(!Cube::from_string("0101").merge_adjacent(Cube::from_string("0000")).has_value());
        CHECK(Cube::from_minterm(4, 5).to_string() == "0101");
        auto i = Cube::from_string("0--1").intersect(Cube::from_string("-1-1"));
        CHECK(i && i->to_string() == "01-1");
    }
    {
        Cube wide = Cube::universe(100);
        wide.set(99, Lit::One);
        wide.set(64, Lit::Zero);
        CHECK(wide.literal_count() == 2);
        Cube m = Cube::universe(100);
        m.set(99, Lit::One);
        CHECK(wide.covers(m) == false);
        CHECK(m.covers(wide));
    }

    return TEST_SUMMARY("test_parser");
}
