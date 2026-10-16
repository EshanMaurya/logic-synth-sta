#include "lss/minimize.hpp"
#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace lss {

namespace {

using Group = std::unordered_map<Cube, bool, CubeHash>;

}

std::vector<Cube> generate_primes(const std::vector<Cube>& minterms) {
    if (minterms.empty()) return {};
    const std::size_t n = minterms.front().num_vars();

    std::vector<Group> groups(n + 1);
    for (const Cube& m : minterms) {
        if (!m.is_minterm()) throw std::invalid_argument("generate_primes expects minterms");
        groups[m.ones_count()].emplace(m, false);
    }

    std::vector<Cube> primes;

    while (true) {
        std::vector<Group> next(n + 1);
        bool any_merge = false;

        for (std::size_t g = 0; g + 1 <= n; ++g) {
            for (auto& [cube, used] : groups[g]) {
                for (std::size_t v = 0; v < n; ++v) {
                    if (cube.get(v) != Lit::Zero) continue;
                    Cube neighbor = cube;
                    neighbor.set(v, Lit::One);
                    auto it = groups[g + 1].find(neighbor);
                    if (it == groups[g + 1].end()) continue;
                    used = true;
                    it->second = true;
                    Cube merged = cube;
                    merged.set(v, Lit::DontCare);
                    next[g].emplace(std::move(merged), false);
                    any_merge = true;
                }
            }
        }

        for (const Group& grp : groups)
            for (const auto& [cube, used] : grp)
                if (!used) primes.push_back(cube);

        if (!any_merge) break;
        groups = std::move(next);
    }

    std::sort(primes.begin(), primes.end());
    primes.erase(std::unique(primes.begin(), primes.end()), primes.end());
    return primes;
}

} // namespace lss
