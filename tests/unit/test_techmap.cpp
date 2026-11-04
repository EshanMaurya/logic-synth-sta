#include "techmap.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

struct ExpectedPattern{
    int Index;
    std::string gateName;
    int num_inputs;
    int root;
};

struct ExpectedMatch{
    int patternIndex;
    std::vector<int> bindings;
};

struct ExpectedCase{
    int circuitNode;
    std::vector<ExpectedMatch> matches; // empty implies no match expected
};

int main(){
    try{
        std::vector<ExpectedPattern> expected = {
            {0, "NAND2", 2, 2},
            {1, "INV",   1, 1},
            {2, "NAND3", 3, 5},
            {3, "NOR2",  2, 5},
            {4, "AND2",  2, 3},
            {5, "OR2",   2, 4},
            {6, "AOI21", 3, 6}
        };

        // ------- TEST FOR build_patterns ------ // 
        std::vector<pattern> patterns = build_patterns();
        int problems = 0;
        if (patterns.size() != expected.size()){
            problems+=1;
            std::cerr<<"ERROR: Patterns vector size found " << patterns.size() << " expected "<< expected.size()<< std::endl;
            return 1;
        }
        int i = 0;
        for (const pattern &p: patterns){
            if (p.gateIndex != expected.at(i).Index){
                problems+=1;
                std::cerr<<"ERROR: Gate Index " << p.gateIndex << " not matching with expected index: " << expected.at(i).Index << std::endl;
            }

            if (p.num_inputs != expected.at(i).num_inputs){
                problems+=1;
                std::cerr<<"ERROR: number of inputs " << p.num_inputs << " not matching with expected number of inputs: " << expected.at(i).num_inputs << std::endl;
            }     
            
            if (p.patternGraph.root != expected.at(i).root){
                problems+=1;
                std::cerr<<"ERROR: pattern root " << p.patternGraph.root << " not matching with expected root: " << expected.at(i).root << std::endl;
            }
            
            if (p.patternName != expected.at(i).gateName){
                problems+=1;
                std::cerr<<" ERROR: pattern name: " << p.patternName <<" not matching with expected name: " << expected.at(i).gateName << std::endl;
            }
            i+=1;
        }


        // ---------- TEST for find_matches ------------- //

        // Sample Circuit
        SubjectGraph circuit;
        circuit.VectorNode = {
            { node::type::PI,   {},     0  },   // node 0
            { node::type::PI,   {},     1  },   // node 1
            { node::type::NAND, {0, 1}, -1 },  // node 2
            { node::type::INV,  {2},    -1 }   // node 3
        };
        circuit.root = 3;
        // Expected matches table

        std::vector<ExpectedCase> cases = {
            { 0, {} },                                   // PI: no pattern root is a PI
            { 1, {} },                                   // PI
            { 2, { {0, {0, 1}} } },                      // NAND2 only
            { 3, { {1, {2}}, {4, {0, 1}} } }             // INV, then AND2
        };
        
        for (ExpectedCase const &c: cases){
            std::vector<match> matches = find_matches(circuit, c.circuitNode, patterns);
            if (matches.size() != c.matches.size()){ // Checking for size mismatch
                std::cerr<<"ERROR: Expected size " << c.matches.size()<< " got size " << matches.size() << std::endl;
                problems+=1;
                continue;
            }

            
            
            int k = 0;
            for (match const &m: matches){
                if (m.patternIndex != c.matches.at(k).patternIndex){
                    std::cerr<<"ERROR: at Circuit node " << c.circuitNode << ", pattern index " << m.patternIndex <<" not matching with expected pattern index " <<c.matches.at(k).patternIndex <<std::endl; 
                    problems+=1;
                }

                if (m.bindings != c.matches.at(k).bindings){
                    std::cerr<<"ERROR: At Circuit node " << c.circuitNode << " bindings of circuit match not equal to bindings of function " <<std::endl;
                    problems +=1;
                }
                k+=1;
            }
        }

        if (problems>0){
            std::cerr << "Number of problems: " << problems << std::endl;
            return 1;
        } else {
            std::cout<<"ALL TESTS PASSED" << std::endl;
            return 0;
        }        
        
        
    } catch(const std::exception &e) { std::cerr << e.what() << std::endl; return 1; }
}