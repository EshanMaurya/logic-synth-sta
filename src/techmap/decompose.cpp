#include <vector>
#include "decompose.hpp"

SubjectGraph decompose(const SOP &function, int num_inputs){
   // Instantiating Basic Structs
    std::vector<node> NodeVector;
    SubjectGraph Result;

    // Creating input nodes
    for (int k = 0; k<num_inputs; k++){
        node PI;
        PI.NodeType = node::type::PI;
        PI.pinIndex = k;
        NodeVector.push_back(PI);
    }


    
    Result.VectorNode = NodeVector;
    return Result;
}

