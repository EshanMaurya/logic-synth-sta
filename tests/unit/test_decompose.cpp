#include "decompose.hpp"
#include "gates.hpp"
#include <iostream>
#include <stdexcept>
int main(){
    try{
    std::vector<Gate> Gates = build_library();
    Gate NAND2;
    bool found = false;
    // Retrieving NAND2 Gate
    for(const Gate &g: Gates){
        if (g.gate_name == "NAND2"){
            NAND2 = g;
            found = true;
        }
    }
    // Error counting
    if (found == false){
        std::cerr<<"ERROR: NAND2 not found in cell library"<<std::endl;
        return 1;
    }
    // Retrieving Input pin number
    int num_inputs = 0;
    for (const pin &p: NAND2.pins){
        if (p.direction == pin::pinDirections::Input){
            num_inputs += 1;
        }
    }

    // Testing the function 
    SubjectGraph GraphResult = decompose(NAND2.function, num_inputs);


    // Checking for further errors
    if (GraphResult.VectorNode.size()!=static_cast<size_t>(num_inputs)){
        std::cerr<<"ERROR: Vector Node size not correct"<<std::endl;
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

    std::cout<<"TESTS PASSED"<<std::endl;

    return 0;
    }
    catch (const std::exception& e) {
        std::cerr<<e.what()<<std::endl;
        return 1;

    }
}