#include "lss/parser.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <memory>
#include <set>
#include <sstream>
#include <unordered_set>

namespace lss {

namespace {

std::string trim(const std::string& s) {
    std::size_t a = 0;
    std::size_t b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

std::vector<std::string> split_ws(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream is(s);
    std::string tok;
    while (is >> tok) out.push_back(tok);
    return out;
}

std::size_t parse_size(const std::string& s, const std::string& what) {
    try {
        std::size_t pos = 0;
        unsigned long long v = std::stoull(s, &pos);
        if (pos != s.size()) throw ParseError("invalid " + what + ": " + s);
        return static_cast<std::size_t>(v);
    } catch (const std::invalid_argument&) {
        throw ParseError("invalid " + what + ": " + s);
    } catch (const std::out_of_range&) {
        throw ParseError("out of range " + what + ": " + s);
    }
}

void require_enumerable(std::size_t n) {
    if (n == 0) throw ParseError("function has no variables");
    if (n > kMaxEnumVars)
        throw ParseError("this input format enumerates the truth table and supports at most " +
                         std::to_string(kMaxEnumVars) + " variables; got " + std::to_string(n));
}

std::vector<Cube> all_minterms(std::size_t n) {
    std::vector<Cube> v;
    v.reserve(static_cast<std::size_t>(1) << n);
    for (std::uint64_t a = 0; a < (1ULL << n); ++a) v.push_back(Cube::from_minterm(n, a));
    return v;
}

} // namespace

std::vector<std::string> split_var_list(const std::string& csv) {
    std::vector<std::string> out;
    std::string cur;
    for (char ch : csv) {
        if (ch == ',' || std::isspace(static_cast<unsigned char>(ch))) {
            if (!cur.empty()) out.push_back(cur);
            cur.clear();
        } else {
            cur += ch;
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

std::vector<BoolFunction> parse_pla(std::istream& in) {
    std::size_t ni = 0;
    std::size_t no = 0;
    std::string type = "fd";
    std::vector<std::string> ilb;
    std::vector<std::string> ob;

    struct Row {
        std::string in;
        std::string out;
        std::size_t line;
    };
    std::vector<Row> rows;

    std::string raw;
    std::size_t lineno = 0;
    while (std::getline(in, raw)) {
        ++lineno;
        std::size_t hash = raw.find('#');
        if (hash != std::string::npos) raw.erase(hash);
        std::string line = trim(raw);
        if (line.empty()) continue;

        if (line[0] == '.') {
            auto toks = split_ws(line);
            const std::string& d = toks[0];
            if (d == ".i") {
                if (toks.size() < 2) throw ParseError("line " + std::to_string(lineno) + ": .i needs a value");
                ni = parse_size(toks[1], ".i");
            } else if (d == ".o") {
                if (toks.size() < 2) throw ParseError("line " + std::to_string(lineno) + ": .o needs a value");
                no = parse_size(toks[1], ".o");
            } else if (d == ".ilb") {
                ilb.assign(toks.begin() + 1, toks.end());
            } else if (d == ".ob") {
                ob.assign(toks.begin() + 1, toks.end());
            } else if (d == ".type") {
                if (toks.size() < 2) throw ParseError("line " + std::to_string(lineno) + ": .type needs a value");
                type = toks[1];
            } else if (d == ".e" || d == ".end") {
                break;
            }
            continue;
        }

        auto toks = split_ws(line);
        Row r;
        r.line = lineno;
        if (toks.size() >= 2) {
            r.in = toks[0];
            for (std::size_t i = 1; i < toks.size(); ++i) r.out += toks[i];
        } else if (ni && no && toks[0].size() == ni + no) {
            r.in = toks[0].substr(0, ni);
            r.out = toks[0].substr(ni);
        } else {
            throw ParseError("line " + std::to_string(lineno) + ": malformed row '" + line + "'");
        }
        rows.push_back(std::move(r));
    }

    if (ni == 0) {
        if (rows.empty()) throw ParseError("PLA has no .i header and no rows");
        ni = rows.front().in.size();
    }
    if (no == 0) {
        if (rows.empty()) throw ParseError("PLA has no .o header and no rows");
        no = rows.front().out.size();
    }
    if (type != "f" && type != "fd" && type != "fr" && type != "fdr")
        throw ParseError("unsupported .type " + type);
    if (!ilb.empty() && ilb.size() != ni) throw ParseError(".ilb count does not match .i");
    if (!ob.empty() && ob.size() != no) throw ParseError(".ob count does not match .o");

    std::vector<BoolFunction> fs(no);
    for (std::size_t o = 0; o < no; ++o) {
        fs[o].var_names = ilb.empty() ? default_var_names(ni) : ilb;
        fs[o].name = ob.empty() ? (no == 1 ? "f" : "f" + std::to_string(o)) : ob[o];
    }
    std::vector<std::vector<Cube>> off(no);

    for (const Row& r : rows) {
        if (r.in.size() != ni)
            throw ParseError("line " + std::to_string(r.line) + ": input part has " + std::to_string(r.in.size()) +
                             " columns, expected " + std::to_string(ni));
        if (r.out.size() != no)
            throw ParseError("line " + std::to_string(r.line) + ": output part has " + std::to_string(r.out.size()) +
                             " columns, expected " + std::to_string(no));
        Cube c;
        try {
            c = Cube::from_string(r.in);
        } catch (const std::invalid_argument& e) {
            throw ParseError("line " + std::to_string(r.line) + ": " + e.what());
        }
        for (std::size_t o = 0; o < no; ++o) {
            char ch = r.out[o];
            switch (ch) {
                case '1':
                case '4':
                    fs[o].on.push_back(c);
                    break;
                case '-':
                case '2':
                    if (type == "fd" || type == "fdr") fs[o].dc.push_back(c);
                    break;
                case '0':
                case '3':
                    if (type == "fr" || type == "fdr") off[o].push_back(c);
                    break;
                case '~':
                    break;
                default:
                    throw ParseError("line " + std::to_string(r.line) + ": invalid output character '" +
                                     std::string(1, ch) + "'");
            }
        }
    }

    if (type == "fr") {
        require_enumerable(ni);
        for (std::size_t o = 0; o < no; ++o) {
            BoolFunction tmp = fs[o];
            tmp.dc = off[o];
            std::unordered_set<Cube, CubeHash> known;
            for (const Cube& m : fs[o].on_minterms()) known.insert(m);
            for (const Cube& m : tmp.dc_minterms()) known.insert(m);
            for (const Cube& m : all_minterms(ni))
                if (!known.count(m)) fs[o].dc.push_back(m);
        }
    }
    return fs;
}

std::vector<BoolFunction> parse_pla_string(const std::string& text) {
    std::istringstream is(text);
    return parse_pla(is);
}

namespace {

std::vector<std::uint64_t> parse_index_list(const std::string& body) {
    std::vector<std::uint64_t> out;
    std::string cur;
    auto flush = [&]() {
        std::string t = trim(cur);
        cur.clear();
        if (t.empty()) return;
        std::size_t dash = t.find('-');
        if (dash != std::string::npos && dash > 0) {
            std::size_t lo = parse_size(trim(t.substr(0, dash)), "minterm");
            std::size_t hi = parse_size(trim(t.substr(dash + 1)), "minterm");
            if (hi < lo) throw ParseError("descending range " + t);
            for (std::size_t v = lo; v <= hi; ++v) out.push_back(v);
        } else {
            out.push_back(parse_size(t, "minterm"));
        }
    };
    for (char ch : body) {
        if (ch == ',') flush();
        else cur += ch;
    }
    flush();
    return out;
}

} // namespace

BoolFunction parse_minterm_list(const std::string& text, const std::vector<std::string>& vars) {
    BoolFunction f;
    std::string s = text;
    std::vector<std::string> header_vars;

    std::size_t eq = s.find('=');
    if (eq != std::string::npos) {
        std::string head = trim(s.substr(0, eq));
        s = s.substr(eq + 1);
        std::size_t lp = head.find('(');
        std::size_t rp = head.rfind(')');
        if (lp != std::string::npos && rp != std::string::npos && rp > lp) {
            std::string nm = trim(head.substr(0, lp));
            if (!nm.empty()) f.name = nm;
            header_vars = split_var_list(head.substr(lp + 1, rp - lp - 1));
        } else if (!head.empty()) {
            f.name = head;
        }
    }

    std::vector<std::uint64_t> minterms;
    std::vector<std::uint64_t> maxterms;
    std::vector<std::uint64_t> dontcares;
    bool have_min = false;
    bool have_max = false;

    std::size_t i = 0;
    while (i < s.size()) {
        char ch = s[i];
        if (std::isspace(static_cast<unsigned char>(ch)) || ch == '+' || ch == ';') {
            ++i;
            continue;
        }
        std::string word;
        while (i < s.size() && (std::isalpha(static_cast<unsigned char>(s[i])) || s[i] == '_')) word += s[i++];
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
        if (word.empty() || i >= s.size() || s[i] != '(')
            throw ParseError("expected m(...), M(...) or d(...) near '" + s.substr(i > 10 ? i - 10 : 0, 20) + "'");
        std::size_t close = s.find(')', i);
        if (close == std::string::npos) throw ParseError("unterminated '(' in minterm list");
        auto idx = parse_index_list(s.substr(i + 1, close - i - 1));
        i = close + 1;

        if (word == "m" || word == "sum" || word == "sigma") {
            minterms.insert(minterms.end(), idx.begin(), idx.end());
            have_min = true;
        } else if (word == "M" || word == "pi" || word == "PI") {
            maxterms.insert(maxterms.end(), idx.begin(), idx.end());
            have_max = true;
        } else if (word == "d" || word == "D" || word == "dc") {
            dontcares.insert(dontcares.end(), idx.begin(), idx.end());
        } else {
            throw ParseError("unknown term list '" + word + "'");
        }
    }

    if (have_min && have_max) throw ParseError("cannot mix m(...) and M(...) in one function");
    if (!have_min && !have_max) throw ParseError("no m(...) or M(...) list found");

    std::vector<std::string> names = !vars.empty() ? vars : header_vars;
    std::uint64_t max_idx = 0;
    for (auto v : minterms) max_idx = std::max(max_idx, v);
    for (auto v : maxterms) max_idx = std::max(max_idx, v);
    for (auto v : dontcares) max_idx = std::max(max_idx, v);
    if (names.empty()) {
        std::size_t n = 1;
        while (n < 63 && (1ULL << n) <= max_idx) ++n;
        names = default_var_names(n);
    }
    std::size_t n = names.size();
    require_enumerable(n);
    if (max_idx >= (1ULL << n))
        throw ParseError("term index " + std::to_string(max_idx) + " does not fit in " + std::to_string(n) +
                         " variables");
    f.var_names = names;

    std::set<std::uint64_t> dcs(dontcares.begin(), dontcares.end());
    if (have_min) {
        std::set<std::uint64_t> ons(minterms.begin(), minterms.end());
        for (auto v : ons) f.on.push_back(Cube::from_minterm(n, v));
        for (auto v : dcs)
            if (!ons.count(v)) f.dc.push_back(Cube::from_minterm(n, v));
    } else {
        std::set<std::uint64_t> offs(maxterms.begin(), maxterms.end());
        for (std::uint64_t a = 0; a < (1ULL << n); ++a) {
            if (dcs.count(a) && !offs.count(a)) f.dc.push_back(Cube::from_minterm(n, a));
            else if (!offs.count(a)) f.on.push_back(Cube::from_minterm(n, a));
        }
    }
    return f;
}

BoolFunction parse_truth_string(const std::string& bits, const std::vector<std::string>& vars) {
    std::string clean;
    for (char ch : bits) {
        if (std::isspace(static_cast<unsigned char>(ch)) || ch == '_' || ch == '|') continue;
        clean += ch;
    }
    if (clean.empty()) throw ParseError("empty truth table");
    std::size_t len = clean.size();
    if ((len & (len - 1)) != 0) throw ParseError("truth table length " + std::to_string(len) + " is not a power of two");
    std::size_t n = 0;
    while ((static_cast<std::size_t>(1) << n) < len) ++n;
    if (n == 0) throw ParseError("truth table needs at least two entries");
    require_enumerable(n);
    if (!vars.empty() && vars.size() != n)
        throw ParseError("truth table over " + std::to_string(n) + " variables but " + std::to_string(vars.size()) +
                         " names given");

    BoolFunction f;
    f.var_names = vars.empty() ? default_var_names(n) : vars;
    for (std::size_t i = 0; i < len; ++i) {
        char ch = clean[i];
        if (ch == '1') f.on.push_back(Cube::from_minterm(n, i));
        else if (ch == 'x' || ch == 'X' || ch == '-') f.dc.push_back(Cube::from_minterm(n, i));
        else if (ch != '0')
            throw ParseError(std::string("invalid truth table character '") + ch + "'");
    }
    return f;
}

namespace {

enum class Tok { Var, Zero, One, Or, And, Xor, Not, Prime, LParen, RParen, End };

struct Token {
    Tok kind;
    std::string text;
    std::size_t pos;
};

std::vector<Token> tokenize(const std::string& s, const std::vector<std::string>& declared) {
    std::vector<std::string> by_len = declared;
    std::sort(by_len.begin(), by_len.end(), [](const std::string& a, const std::string& b) {
        return a.size() > b.size();
    });

    std::vector<Token> out;
    std::size_t i = 0;
    while (i < s.size()) {
        char ch = s[i];
        if (std::isspace(static_cast<unsigned char>(ch))) {
            ++i;
            continue;
        }
        if (ch == '+' || ch == '|') out.push_back({Tok::Or, std::string(1, ch), i++});
        else if (ch == '*' || ch == '&' || ch == '.') out.push_back({Tok::And, std::string(1, ch), i++});
        else if (ch == '^') out.push_back({Tok::Xor, "^", i++});
        else if (ch == '!' || ch == '~') out.push_back({Tok::Not, std::string(1, ch), i++});
        else if (ch == '\'') out.push_back({Tok::Prime, "'", i++});
        else if (ch == '(') out.push_back({Tok::LParen, "(", i++});
        else if (ch == ')') out.push_back({Tok::RParen, ")", i++});
        else if (ch == '0') out.push_back({Tok::Zero, "0", i++});
        else if (ch == '1') out.push_back({Tok::One, "1", i++});
        else if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
            std::size_t start = i;
            bool matched = false;
            for (const std::string& name : by_len) {
                if (s.compare(i, name.size(), name) == 0) {
                    out.push_back({Tok::Var, name, start});
                    i += name.size();
                    matched = true;
                    break;
                }
            }
            if (matched) continue;
            if (!declared.empty())
                throw ParseError("unknown variable near position " + std::to_string(i) + " in '" + s + "'");
            std::string name(1, s[i++]);
            while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) name += s[i++];
            out.push_back({Tok::Var, name, start});
        } else {
            throw ParseError(std::string("unexpected character '") + ch + "' at position " + std::to_string(i));
        }
    }
    out.push_back({Tok::End, "", s.size()});
    return out;
}

struct Node {
    enum Kind { Var, Const, Not, And, Or, Xor } kind;
    std::size_t var = 0;
    bool value = false;
    int a = -1;
    int b = -1;
};

class ExprParser {
public:
    ExprParser(std::vector<Token> toks, const std::vector<std::string>& vars) : toks_(std::move(toks)), vars_(vars) {}

    int parse() {
        int root = parse_or();
        if (peek().kind != Tok::End) fail("unexpected token '" + peek().text + "'");
        return root;
    }

    const std::vector<Node>& nodes() const { return nodes_; }

    bool eval(int id, std::uint64_t a, std::size_t n) const {
        const Node& nd = nodes_[static_cast<std::size_t>(id)];
        switch (nd.kind) {
            case Node::Var: return (a >> (n - 1 - nd.var)) & 1ULL;
            case Node::Const: return nd.value;
            case Node::Not: return !eval(nd.a, a, n);
            case Node::And: return eval(nd.a, a, n) && eval(nd.b, a, n);
            case Node::Or: return eval(nd.a, a, n) || eval(nd.b, a, n);
            case Node::Xor: return eval(nd.a, a, n) != eval(nd.b, a, n);
        }
        return false;
    }

private:
    [[noreturn]] void fail(const std::string& msg) const {
        throw ParseError(msg + " at position " + std::to_string(peek().pos));
    }
    const Token& peek() const { return toks_[pos_]; }
    Token next() { return toks_[pos_++]; }

    int add(Node n) {
        nodes_.push_back(n);
        return static_cast<int>(nodes_.size() - 1);
    }

    bool starts_factor() const {
        Tok k = peek().kind;
        return k == Tok::Var || k == Tok::Zero || k == Tok::One || k == Tok::LParen || k == Tok::Not;
    }

    int parse_or() {
        int l = parse_xor();
        while (peek().kind == Tok::Or) {
            next();
            int r = parse_xor();
            Node n{Node::Or};
            n.a = l;
            n.b = r;
            l = add(n);
        }
        return l;
    }

    int parse_xor() {
        int l = parse_and();
        while (peek().kind == Tok::Xor) {
            next();
            int r = parse_and();
            Node n{Node::Xor};
            n.a = l;
            n.b = r;
            l = add(n);
        }
        return l;
    }

    int parse_and() {
        int l = parse_unary();
        while (true) {
            if (peek().kind == Tok::And) next();
            else if (!starts_factor()) break;
            int r = parse_unary();
            Node n{Node::And};
            n.a = l;
            n.b = r;
            l = add(n);
        }
        return l;
    }

    int parse_unary() {
        if (peek().kind == Tok::Not) {
            next();
            int c = parse_unary();
            Node n{Node::Not};
            n.a = c;
            return add(n);
        }
        return parse_postfix();
    }

    int parse_postfix() {
        int p = parse_primary();
        while (peek().kind == Tok::Prime) {
            next();
            Node n{Node::Not};
            n.a = p;
            p = add(n);
        }
        return p;
    }

    int parse_primary() {
        Token t = peek();
        if (t.kind == Tok::Var) {
            next();
            auto it = std::find(vars_.begin(), vars_.end(), t.text);
            Node n{Node::Var};
            n.var = static_cast<std::size_t>(it - vars_.begin());
            return add(n);
        }
        if (t.kind == Tok::Zero || t.kind == Tok::One) {
            next();
            Node n{Node::Const};
            n.value = t.kind == Tok::One;
            return add(n);
        }
        if (t.kind == Tok::LParen) {
            next();
            int e = parse_or();
            if (peek().kind != Tok::RParen) fail("expected ')'");
            next();
            return e;
        }
        fail("expected a variable, constant or '('");
    }

    std::vector<Token> toks_;
    const std::vector<std::string>& vars_;
    std::size_t pos_ = 0;
    std::vector<Node> nodes_;
};

bool var_less(const std::string& a, const std::string& b) {
    auto split = [](const std::string& s) {
        std::size_t i = 0;
        while (i < s.size() && !std::isdigit(static_cast<unsigned char>(s[i]))) ++i;
        std::string head = s.substr(0, i);
        unsigned long long num = i < s.size() ? std::stoull(s.substr(i)) : 0;
        return std::make_pair(head, num);
    };
    return split(a) < split(b);
}

} // namespace

BoolFunction parse_expression(const std::string& expr, const std::string& dc_expr,
                              const std::vector<std::string>& vars) {
    std::vector<Token> t_on = tokenize(expr, vars);
    std::vector<Token> t_dc;
    if (!trim(dc_expr).empty()) t_dc = tokenize(dc_expr, vars);

    std::vector<std::string> names = vars;
    if (names.empty()) {
        std::set<std::string, decltype(&var_less)> seen(&var_less);
        for (const Token& t : t_on)
            if (t.kind == Tok::Var) seen.insert(t.text);
        for (const Token& t : t_dc)
            if (t.kind == Tok::Var) seen.insert(t.text);
        names.assign(seen.begin(), seen.end());
    }
    std::size_t n = names.size();
    require_enumerable(n);

    ExprParser on_parser(std::move(t_on), names);
    int on_root = on_parser.parse();

    std::unique_ptr<ExprParser> dc_parser;
    int dc_root = -1;
    if (!t_dc.empty()) {
        dc_parser = std::make_unique<ExprParser>(std::move(t_dc), names);
        dc_root = dc_parser->parse();
    }

    BoolFunction f;
    f.var_names = names;
    for (std::uint64_t a = 0; a < (1ULL << n); ++a) {
        if (on_parser.eval(on_root, a, n)) f.on.push_back(Cube::from_minterm(n, a));
        else if (dc_parser && dc_parser->eval(dc_root, a, n)) f.dc.push_back(Cube::from_minterm(n, a));
    }
    return f;
}

std::vector<BoolFunction> parse_file(const std::string& path, const std::vector<std::string>& vars) {
    std::ifstream in(path);
    if (!in) throw ParseError("cannot open " + path);
    std::stringstream buf;
    buf << in.rdbuf();
    std::string text = buf.str();

    std::string stripped;
    {
        std::istringstream is(text);
        std::string line;
        while (std::getline(is, line)) {
            std::size_t h = line.find('#');
            if (h != std::string::npos) line.erase(h);
            if (!trim(line).empty()) stripped += line + "\n";
        }
    }
    std::string first = trim(stripped);
    if (first.empty()) throw ParseError(path + " is empty");

    if (first[0] == '.') return parse_pla_string(text);

    bool has_terms = false;
    for (std::size_t i = 0; i + 1 < first.size(); ++i) {
        if ((first[i] == 'm' || first[i] == 'M') && first[i + 1] == '(' &&
            (i == 0 || !std::isalnum(static_cast<unsigned char>(first[i - 1])))) {
            has_terms = true;
            break;
        }
    }
    if (has_terms) return {parse_minterm_list(stripped, vars)};

    std::string expr = stripped;
    std::string name = "f";
    std::size_t eq = expr.find('=');
    if (eq != std::string::npos) {
        std::string head = trim(expr.substr(0, eq));
        if (!head.empty() && head.find_first_of("+*&|^'()!~") == std::string::npos) name = head;
        expr = expr.substr(eq + 1);
    }
    std::string dc;
    std::size_t semi = expr.find(';');
    if (semi != std::string::npos) {
        std::string rest = trim(expr.substr(semi + 1));
        expr = expr.substr(0, semi);
        std::size_t colon = rest.find('=');
        if (rest.rfind("dc", 0) == 0 && colon != std::string::npos) dc = rest.substr(colon + 1);
    }
    BoolFunction f = parse_expression(expr, dc, vars);
    f.name = name;
    return {f};
}

} // namespace lss
