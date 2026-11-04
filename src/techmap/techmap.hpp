#pragma once
#include <vector>
#include "cell_library.hpp"
#include "decompose.hpp"
#include <string>

struct pattern{
    std::string patternName;
    int gateIndex; // for pointing back to cell_library
    SubjectGraph patternGraph;
    int num_inputs;
    int area;
};

std::vector<pattern> build_patterns();

struct match{
    int patternIndex;
    std::vector<int> bindings;
};

std::vector<match> find_matches(const SubjectGraph &circuitGraph, int circuitNode, const std::vector<pattern> &patterns);