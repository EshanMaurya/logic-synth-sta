#include "lss/minimize.hpp"
#include "lss/parser.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

using namespace lss;

namespace {

struct Args {
    std::string file;
    std::string expr;
    std::string dc;
    std::string minterms;
    std::string truth;
    std::string vars;
    bool pos = false;
    bool primes = false;
    bool json = false;
    bool have_output = false;
    std::size_t output = 0;
    std::size_t petrick_limit = 5000;
};

void usage() {
    std::cerr << "usage: lss_minimize [options] [file]\n"
                 "  file                 .pla, minterm list (f(a,b,c)=m(...)+d(...)) or expression file\n"
                 "  -e <expr>            SOP/POS expression, e.g. \"a'b + cd\" or \"(a+b')(c+d)\"\n"
                 "  --dc <expr>          don't-care expression (with -e)\n"
                 "  -m <list>            minterm list, e.g. \"f(a,b,c,d)=m(0,1,2,5)+d(3,7)\"\n"
                 "  --tt <bits>          truth table, MSB-first row order, x or - for don't care\n"
                 "  --vars a,b,c         variable names / order\n"
                 "  --output N           select output N of a multi-output PLA\n"
                 "  --pos                also print the minimal product-of-sums form\n"
                 "  --primes             list all prime implicants\n"
                 "  --petrick-limit N    max Petrick product terms before switching to branch-and-bound\n"
                 "  --json               machine-readable output\n";
}

std::string json_escape(const std::string& s) {
    std::string o;
    for (char c : s) {
        if (c == '"' || c == '\\') o += '\\';
        o += c;
    }
    return o;
}

void print_json(const BoolFunction& f, const MinimizeResult& r, const MinimizeResult* pos) {
    std::cout << "{\"name\":\"" << json_escape(f.name) << "\",\"vars\":[";
    for (std::size_t i = 0; i < f.var_names.size(); ++i)
        std::cout << (i ? "," : "") << "\"" << json_escape(f.var_names[i]) << "\"";
    std::cout << "],\"cubes\":[";
    for (std::size_t i = 0; i < r.cover.size(); ++i)
        std::cout << (i ? "," : "") << "\"" << r.cover[i].to_string() << "\"";
    std::cout << "],\"primes\":[";
    for (std::size_t i = 0; i < r.primes.size(); ++i)
        std::cout << (i ? "," : "") << "\"" << r.primes[i].to_string() << "\"";
    std::cout << "],\"essential\":" << r.essential.size() << ",\"terms\":" << r.term_count
              << ",\"literals\":" << r.literal_count << ",\"exact\":" << (r.exact ? "true" : "false");
    if (pos) {
        std::cout << ",\"pos_terms\":" << pos->term_count << ",\"pos_literals\":" << pos->literal_count;
    }
    std::cout << "}\n";
}

void print_text(const BoolFunction& f, const MinimizeResult& r, const MinimizeResult* pos, bool list_primes) {
    std::cout << f.name << "(";
    for (std::size_t i = 0; i < f.var_names.size(); ++i) std::cout << (i ? "," : "") << f.var_names[i];
    std::cout << ")\n";
    std::cout << "  on-set minterms : " << f.on_minterms().size() << "\n";
    std::cout << "  don't-cares     : " << f.dc_minterms().size() << "\n";
    std::cout << "  prime implicants: " << r.primes.size() << "\n";
    std::cout << "  essential primes: " << r.essential.size() << "\n";
    if (list_primes)
        for (const Cube& p : r.primes) std::cout << "    " << p.to_string() << "  " << p.to_product(f.var_names) << "\n";
    std::cout << "  cover           : " << r.term_count << " terms, " << r.literal_count << " literals"
              << (r.exact ? "" : "  (search budget exhausted, best cover found, not proven minimal)") << "\n";
    std::cout << "  SOP             : " << f.name << " = " << format_sop(r.cover, f.var_names) << "\n";
    if (pos)
        std::cout << "  POS             : " << f.name << " = " << format_pos(pos->cover, f.var_names) << "  ("
                  << pos->term_count << " sums, " << pos->literal_count << " literals)\n";
}

} // namespace

int main(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        auto need = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "missing value for " << flag << "\n";
                usage();
                std::exit(2);
            }
            return argv[++i];
        };
        if (s == "-e") a.expr = need("-e");
        else if (s == "--dc") a.dc = need("--dc");
        else if (s == "-m") a.minterms = need("-m");
        else if (s == "--tt") a.truth = need("--tt");
        else if (s == "--vars") a.vars = need("--vars");
        else if (s == "--output") {
            a.output = static_cast<std::size_t>(std::stoul(need("--output")));
            a.have_output = true;
        } else if (s == "--pos") a.pos = true;
        else if (s == "--primes") a.primes = true;
        else if (s == "--json") a.json = true;
        else if (s == "--petrick-limit") a.petrick_limit = static_cast<std::size_t>(std::stoull(need("--petrick-limit")));
        else if (s == "-h" || s == "--help") {
            usage();
            return 0;
        } else if (!s.empty() && s[0] == '-' && s != "-") {
            std::cerr << "unknown option " << s << "\n";
            usage();
            return 2;
        } else a.file = s;
    }

    try {
        std::vector<std::string> vars = split_var_list(a.vars);
        std::vector<BoolFunction> fs;
        if (!a.expr.empty()) fs.push_back(parse_expression(a.expr, a.dc, vars));
        else if (!a.minterms.empty()) fs.push_back(parse_minterm_list(a.minterms, vars));
        else if (!a.truth.empty()) fs.push_back(parse_truth_string(a.truth, vars));
        else if (!a.file.empty()) fs = parse_file(a.file, vars);
        else {
            usage();
            return 2;
        }

        if (a.have_output) {
            if (a.output >= fs.size()) {
                std::cerr << "output index out of range (file has " << fs.size() << ")\n";
                return 2;
            }
            fs = {fs[a.output]};
        }

        MinimizeOptions opts;
        opts.petrick_term_limit = a.petrick_limit;

        bool ok = true;
        for (const BoolFunction& f : fs) {
            MinimizeResult r = minimize_exact(f, opts);
            if (!verify_cover(f, r.cover)) {
                std::cerr << "internal error: cover for " << f.name << " failed verification\n";
                ok = false;
                continue;
            }
            MinimizeResult pos_r;
            bool have_pos = false;
            if (a.pos) {
                pos_r = minimize_exact(f.complement(), opts);
                have_pos = true;
            }
            if (a.json) print_json(f, r, have_pos ? &pos_r : nullptr);
            else print_text(f, r, have_pos ? &pos_r : nullptr, a.primes);
        }
        return ok ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
