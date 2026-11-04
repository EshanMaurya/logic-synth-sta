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
        std::vector<pattern> patterns = build_patterns();
        int problems = 0;
        if (patterns.size() != expected.size()){
            problems+=1;
            std::cerr<<"Patterns vector size found " << patterns.size() << " expected "<< expected.size()<< std::endl;
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
        if (problems>0){
            std::cerr << "Number of problems: " << problems << std::endl;
            return 1;
        } else {
            std::cout<<"ALL TESTS PASSED" << std::endl;
            return 0;
        }
        
    } catch(const std::exception &e) { std::cerr << e.what() << std::endl; return 1; }
}