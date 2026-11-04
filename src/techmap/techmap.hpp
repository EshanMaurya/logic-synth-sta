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
    int patternIndex = -1; // -1 means "no match"
    std::vector<int> bindings;
};

std::vector<match> find_matches(const SubjectGraph &circuitGraph, int circuitNode, const std::vector<pattern> &patterns);

struct tmap_result{
    int bestCost = 1000000; // large number 
    match bestMatch;
};

std::vector<tmap_result> dp_cover (const SubjectGraph &circuit, const std::vector<pattern> &patterns);

struct mapped_cell{
    int patternIndex;
    int outputNode;
    std::vector<int> inputNodes;
};

std::vector<mapped_cell> top_down_extractor (const std::vector<tmap_result> &resultTable, int rootNode);