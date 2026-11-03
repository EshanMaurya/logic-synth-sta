#include "decompose.hpp"
#include "gates.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct ExpectedNode{
    std::vector<int> Inputs;
    node::type NodeType;
};

Gate find_gate(std::string const &GateName){
    std::vector<Gate> Gates = build_library(); // This may be an optimization bottleneck

    for (const Gate&g : Gates){
        if (g.gate_name == GateName){
            return g;
        }
    }
    // Incase Gate is not found
    throw std::runtime_error("Gate not found in cell library: " + GateName);

}
int check_gate(std::string const &GateName,std::vector<ExpectedNode> const &ExpectedNodes, int ExpectedRoot){
    std::cerr<<"--------" << GateName << "--------" << std::endl;
    Gate RequiredGate = find_gate(GateName);

    // Counting Input Pins
    int num_inputs = 0;
    for (const pin &p: RequiredGate.pins){
        if (p.direction == pin::pinDirections::Input){
            num_inputs += 1;
        }
    }
    SubjectGraph GraphResult = decompose(RequiredGate.function, num_inputs);
    if (GraphResult.VectorNode.size()!=static_cast<size_t>(num_inputs)+ExpectedNodes.size()){
        std::cerr<<"ERROR: Expected Node size " << num_inputs + ExpectedNodes.size() << ", got " << GraphResult.VectorNode.size() <<std::endl;
        return 1;
    }

    // Check for Nodetype mismatch
    // Checking each node: type, pinIndex, and Inputs
    for (int i = 0; i < num_inputs; i++){
        const node &n = GraphResult.VectorNode.at(i);
        if (n.NodeType != node::type::PI){
            std::cerr<<"ERROR: Node "<<i<<" should be a PI"<<std::endl;
            return 1;
        }
        if (n.pinIndex != i){
            std::cerr<<"ERROR: Node "<<i<<" has pinIndex "<<n.pinIndex<<", expected "<<i<<std::endl;
            return 1;
        }
        if (!n.Inputs.empty()){
            std::cerr<<"ERROR: node "<<i<<" is a PI but has Inputs"<<std::endl;
            return 1;
        }
    }

    for (int j = 0; j<static_cast<int>(ExpectedNodes.size()); j++){
        const node &n = GraphResult.VectorNode.at(j+num_inputs);
        if (n.NodeType != ExpectedNodes.at(j).NodeType){
            std::cerr<<"ERROR: node "<<j + num_inputs<<" is the wrong node type"<< std::endl;
            return 1;
        }
        if (n.Inputs != ExpectedNodes.at(j).Inputs){
            std::cerr<<"ERROR: Expected inputs not matching with resultant inputs for node: " << j+ num_inputs << std::endl;
            return 1;
        }
    }

    if (GraphResult.root != ExpectedRoot){
        std::cerr<<"ERROR: Expected root: "<< ExpectedRoot  << " got: " << GraphResult.root << std::endl;
        return 1;
    }
    std::cerr<<"All Tests Passed for this gate" << std::endl;
    return 0;
}

int main(){
    try{
        int problems = 0;
        problems += check_gate("NAND2", {ExpectedNode{{0,1}, node::type::NAND}}, 2);
        problems += check_gate("NAND3",
            {ExpectedNode{{0,1}, node::type::NAND},
            ExpectedNode{{3},   node::type::INV},
            ExpectedNode{{4,2}, node::type::NAND}},       
            5);

        problems += check_gate("INV",  {ExpectedNode{{0}, node::type::INV}}, 1);
        problems += check_gate("AND2", {ExpectedNode{{0,1}, node::type::NAND},
            ExpectedNode{{2},   node::type::INV}}, 3);
        problems += check_gate("OR2",  {ExpectedNode{{0},   node::type::INV},
            ExpectedNode{{1},   node::type::INV},
            ExpectedNode{{2,3}, node::type::NAND}}, 4);
        problems += check_gate("NOR2", {ExpectedNode{{0},   node::type::INV},
            ExpectedNode{{1},   node::type::INV},
            ExpectedNode{{2,3}, node::type::NAND},
            ExpectedNode{{4},   node::type::INV}}, 5);
        problems += check_gate("AOI21",{ExpectedNode{{0,1}, node::type::NAND},
            ExpectedNode{{2},   node::type::INV},
            ExpectedNode{{3,4}, node::type::NAND},
            ExpectedNode{{5},   node::type::INV}}, 6);
        if (problems>0){
            std::cerr<<"Number of problems: " << problems<< std::endl;
            return 1;
        } else{
            std::cout<<"ALL TESTS PASSED" << std::endl;
            return 0;
        }
            
    } catch(const std::exception &e) { std::cerr << e.what() << std::endl; return 1; }
}