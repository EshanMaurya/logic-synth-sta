#include "sop_eval.hpp"
#include <cmath>
#include "gates.hpp"

bool sop_eval(const SOP& function,const std::vector<bool>& inputs){
    std::vector<bool> AND_accumulator;
    for (const Term &t :function.Terms){
        bool AND_VALUE = true;
            
        for (const Literal &l: t.Literals){
            if(l.polarity == Literal::LiteralPolarity::Normal){
                AND_VALUE = AND_VALUE&&(inputs.at(l.pinIndex));
            }
            else{
                AND_VALUE = AND_VALUE&&(!inputs.at(l.pinIndex));
            }
                
        }
        AND_accumulator.push_back(AND_VALUE);
    }    
    bool OR_accumulator = false;
    for (bool i : AND_accumulator){
        OR_accumulator = OR_accumulator||i;
    }
    if (function.Inverted_Flag == SOP::InvertedFlag::Inverted){
        OR_accumulator = !OR_accumulator;
    }
    return OR_accumulator;
};