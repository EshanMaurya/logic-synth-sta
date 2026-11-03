#include <vector>
#include "decompose.hpp"
#include <utility>
#include <stdexcept>
#include <string>


static int literal_node(std::vector<node> &InputVector, const Literal &literal){
    if (literal.polarity== Literal::LiteralPolarity::Normal){
        return literal.pinIndex;
    } else{
        node N;
        N.Inputs.push_back(literal.pinIndex);
        N.NodeType = node::type::INV;
        InputVector.push_back(N);
        return static_cast<int>(InputVector.size()) - 1;
    }
}

static int build_AND_chain(std::vector<node> &InputVector,const Term &T){
    int acc = literal_node(InputVector,T.Literals.at(0));
    int lastpos=acc;
    for (int k = 1; k<T.Literals.size(); k++){
        int pos = literal_node(InputVector,T.Literals.at(k));
        node N;
        N.NodeType = node::type::NAND;
        N.Inputs = {acc,pos};
        InputVector.push_back(N);
        int nandpos = static_cast<int>(InputVector.size())-1;
        if (k == T.Literals.size()-1){
            lastpos = nandpos;
        } else{
            node inv_node;
            inv_node.NodeType = node::type::INV;
            inv_node.Inputs = {nandpos};
            InputVector.push_back(inv_node);
            acc = static_cast<int>(InputVector.size()) - 1;
        }
        
    }
    return lastpos;
}

static int build_term_complement(std::vector<node> &InputVector, const Term &T){
    node N;
    if (T.Literals.size() == 0){
        throw std::invalid_argument("ERROR: Wrong number of Literals. Zero literals found");
    }
    
    if (T.Literals.size() == 1){
        if (T.Literals.at(0).polarity == Literal::LiteralPolarity::Normal){
            N.NodeType = node::type::INV;
            N.Inputs.push_back(T.Literals.at(0).pinIndex);
            InputVector.push_back(N);
        } else{
            return T.Literals.at(0).pinIndex;
        }

    } else{
        return build_AND_chain(InputVector,T);
    }
    
    return static_cast<int>(InputVector.size())-1;
}



SubjectGraph decompose(const SOP &function, int num_inputs){
   // Instantiating Basic Struct
    SubjectGraph Result;

    // Creating input nodes
    for (int k = 0; k<num_inputs; k++){
        node PI;
        PI.NodeType = node::type::PI;
        PI.pinIndex = k;
        Result.VectorNode.push_back(PI);
    }

    std::vector<int> comps; //Complements
    for (const Term &t: function.Terms){
        int pos = build_term_complement(Result.VectorNode, t);
        comps.push_back(pos);
    }

    if (comps.size() == 1){
        if (function.Inverted_Flag == SOP::InvertedFlag::Inverted){
            Result.root = comps.at(0);
        } else{
            node N;
            N.NodeType = node::type::INV;
            N.Inputs = {comps.at(0)};
            Result.VectorNode.push_back(N);
            Result.root = static_cast<int>(Result.VectorNode.size()-1);
        }
    } else if(comps.size()==2){
        node N;
        N.NodeType = node::type::NAND;
        N.Inputs = {comps.at(0), comps.at(1)};
        Result.VectorNode.push_back(N);
        if( function.Inverted_Flag == SOP::InvertedFlag::Normal){
            Result.root = static_cast<int>(Result.VectorNode.size()-1);
        } else{
            node inv_node;
            inv_node.NodeType = node::type::INV;
            inv_node.Inputs = {static_cast<int>((Result.VectorNode.size()-1))};
            Result.VectorNode.push_back(inv_node);
            Result.root = static_cast<int>(Result.VectorNode.size()-1);
        } 
    } else if(comps.size() == 0){
        throw std::invalid_argument("Zero number of compliments found");
    } else if(comps.size()>2){
        throw std::invalid_argument("More than 2 compliments not supported currently.");
    }
    return Result;
}

bool evaluate_graph(const SubjectGraph &Graph, const std::vector<bool> &row){
    std::vector<bool> value;

    for (node const &N: Graph.VectorNode){
        

        if (N.NodeType  == node::type::PI){
            value.push_back(row.at(N.pinIndex));
        } else if(N.NodeType == node::type::INV){
            value.push_back(!(value.at(N.Inputs.at(0))));
        } else if(N.NodeType == node::type::NAND){
            value.push_back(!(value.at(N.Inputs.at(0))&&value.at(N.Inputs.at(1))));
        } else{
            throw std::invalid_argument("Unknown node type");
        }

    }
    return value.at(Graph.root);
}
