#include "cell_library.hpp"
#include "build_gate.hpp"
// To compute NOT (A AND B) = NOT A OR NOT B

Term term1 = {
    {
        {"A", Literal::LiteralPolarity::Normal},
        {"B", Literal::LiteralPolarity::Normal}
    }
};


SOP sop = {
    {term1}, 
    SOP::InvertedFlag::Inverted
};

pin A = {"A",pin::pinDirections::Input,13}; 
pin B = {"B", pin::pinDirections::Input, 14};
pin Y = {"Y", pin::pinDirections::Output, 0}; // Note zero here does not imply that the output pin has ANY capacitance.

Gate NAND2 = build_gate(
    sop,
    {A, B, Y},
    {
        {"X1", 14, {{14,13}, {15,16}}},
        {"X2", 15, {{14,13}, {15,16}}}
    },
    "NAND2"
);

int main(){
    std::cout << NAND2.pins[0].name <<std::endl; // Accessing
    std::cout << NAND2.function.Terms.size() << std::endl; // Accessing Size
    for (const Term t: NAND2.function.Terms){
        for (const Literal l:t.Literals){
            std::cout<< l.literal_name << "->"<< l.pinIndex<< std::endl;
        }
    }
    
    return 0;
}