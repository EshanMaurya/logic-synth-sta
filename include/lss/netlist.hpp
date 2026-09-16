#pragma once
// Shared graph/DAG data structures representing a Boolean network / gate-level netlist.
// Used by minimization, techmap, and sta modules.

namespace lss {

class Netlist {
public:
    Netlist() = default;
    // TODO: nodes (gates/literals), edges (signal connections), I/O ports
};

} // namespace lss
