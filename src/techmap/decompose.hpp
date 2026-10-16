#pragma once
#include <vector>
#include "cell_library.hpp"

struct node{
    enum class type {PI, NAND, INV};
    type NodeType;
    std::vector<int> Inputs;
    int pinIndex = -1; // Only applicable for PI's.
};

struct SubjectGraph{
    std::vector<node> VectorNode;
    int root = -1;
};

SubjectGraph decompose(const SOP &function, int num_inputs);