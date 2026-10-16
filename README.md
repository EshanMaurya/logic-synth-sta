# logic-synth-sta

A scaled-down logic synthesis and static timing analysis (STA) engine — the
same pipeline Synopsys Design Compiler / Yosys run internally, built from
scratch and benchmarked like an EDA vendor would.

## Pipeline

1. **Minimization** (`src/minimization/`) — Quine-McCluskey + Petrick's method
   for exact minimization, then the ESPRESSO heuristic (expand / reduce /
   irredundant cover) for circuits too large for exact methods.
2. **Technology mapping** (`src/techmap/`) — converts the minimized Boolean
   network into a gate-level netlist restricted to a target cell library
   (`cell_library/`: NAND2, NOR2, INV, AOI21) with area/delay characteristics.
3. **Static timing analysis** (`src/sta/`) — topological levelization over the
   mapped netlist; reports worst-case delay and slack on the critical path.

## Benchmarking

Circuits from the ISCAS-85 suite (`benchmarks/iscas85/`) are run through the
full pipeline. Results — % area reduction and critical-path delay vs. an
unoptimized baseline — are tracked in `docs/benchmark_results.md`.

`scripts/yosys_diff.py` cross-checks this engine's synthesis output against
Yosys on the same input for verification. `scripts/gen_schematic.py` renders
the resulting netlist as a schematic (networkx/graphviz) for visual sanity
checks.

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
ctest
```

## Minimizer CLI

```bash
build/src/minimization/lss_minimize -m "f(a,b,c,d)=m(4,8,10,11,12,15)+d(9,14)" --pos
build/src/minimization/lss_minimize -e "(a+b')(c+d)"
build/src/minimization/lss_minimize --tt "0110 100x"
build/src/minimization/lss_minimize tests/data/full_adder.pla
```

Inputs: Espresso-style PLA (multi-output, `.type f|fd|fr|fdr`), minterm/maxterm
lists with don't-cares, SOP/POS expressions (`'`, `!`, `~` for NOT; `+`/`|` OR;
implicit/`*`/`&` AND; `^` XOR), and raw truth-table strings. Variable 0 is the
most significant bit of the minterm index.

Cost order is (product terms, literals). Prime implicants come from QM, the cover
is solved by essential extraction, row/column dominance, then Petrick's method.
If Petrick's product grows past `--petrick-limit`, a branch-and-bound search takes
over; if that hits its node budget the result is flagged `exact: false`.

`scripts/verify_pyeda.py` checks every cover for equivalence and compares cost
against pyeda's espresso (`pip install -r scripts/requirements-verify.txt`).

## Status

- Minimization: Quine-McCluskey + Petrick's method, parsing, CLI and tests done.
  `espresso.cpp` (expand / reduce / irredundant) is the next milestone.
- Techmap, STA, netlist: stubbed.
