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

## Status

Early scaffold — modules are stubbed out; implementation in progress.
