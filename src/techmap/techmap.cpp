#include <iostream>
#include <vector>
#include "gates.hpp"
#include "techmap.hpp"
#include "decompose.hpp"
#include <algorithm>


static int count_inputs(const Gate &g){
    int num_inputs = static_cast<int>(std::count_if(g.pins.begin(), g.pins.end(),
            [](const pin &p){ return p.direction == pin::pinDirections::Input; }));
        return num_inputs;
}
std::vector<pattern> build_patterns(){
    std::vector<Gate> Gates = build_library();
    int i = 0;
    std::vector<pattern> patternvector;
    for (const Gate &g: Gates){
        int num_inputs = count_inputs(g);
        SubjectGraph graph = decompose(g.function, num_inputs);
        pattern p;
        p.area = g.Variants.at(0).area; // Using any variant for now, will fix later
        p.gateIndex = i;
        p.patternGraph = graph;
        p.patternName = g.gate_name;
        p.num_inputs = num_inputs;
        i += 1;
        patternvector.push_back(p);
    }
    return patternvector;
}
