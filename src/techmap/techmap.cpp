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

static bool match_here(const SubjectGraph &patternGraph, const int &patternNode, const SubjectGraph &circuitGraph, const int &circuitNode, std::vector<int> &bindings){
    const node &pnode = patternGraph.VectorNode.at(patternNode);
    node::type ptype = pnode.NodeType;


    const node &cnode = circuitGraph.VectorNode.at(circuitNode);
    node::type ctype = cnode.NodeType;


    if (ptype == node::type::PI){
        int pinNumber = pnode.pinIndex;
        if (bindings.at(pinNumber)==(-1)){
            bindings.at(pinNumber) = circuitNode;
            return true;
        } else return bindings.at(pinNumber) == circuitNode;     
    } else if (ptype == node::type::INV){
        if (ctype != ptype){
            return false;
        }
        return match_here(patternGraph, pnode.Inputs.at(0), circuitGraph, cnode.Inputs.at(0), bindings);// Recursively
    } else if (ptype == node::type::NAND){
        if (ctype != node::type::NAND){
            return false;
        }
        std::vector<int> bindingscopy = bindings;
        if (match_here(patternGraph, pnode.Inputs.at(0), circuitGraph,cnode.Inputs.at(0), bindings) && match_here(patternGraph, pnode.Inputs.at(1), circuitGraph,cnode.Inputs.at(1), bindings)){ // Straight order
            return true;
        } else bindings = bindingscopy;

        if (match_here(patternGraph, pnode.Inputs.at(0), circuitGraph,cnode.Inputs.at(1), bindings) && match_here(patternGraph, pnode.Inputs.at(1), circuitGraph,cnode.Inputs.at(0), bindings)){ // Crossed order
            return true;
        } else {
            bindings = bindingscopy; 
            return false;
        }
    } else{
        throw std::invalid_argument("Unknown node type");
    }
}

std::vector<match> find_matches(const SubjectGraph &circuitGraph, int circuitNode, const std::vector<pattern> &patterns){
    std::vector<match> result;
    int i = 0;
    for (const pattern &p: patterns){
        std::vector<int> bindings(p.num_inputs,-1);
        if (match_here(p.patternGraph, p.patternGraph.root,circuitGraph, circuitNode, bindings)){
            match m;
            m.patternIndex = i;
            m.bindings = bindings;
            result.push_back(m);
        }
        i +=1;
    }
    return result;
}