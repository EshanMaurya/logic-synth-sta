#include "cell_library.hpp"
#include "build_gate.hpp"
// To compute NOT (A AND B) = NOT A OR NOT B

Term term1 = {
    {
        {"A", Negated},
        {"B", Negated}
    }
};

Term term2 = {
    {
        {"A", Normal},
        {"B", Negated}
    }
};

Term term3 = {
    {
        {"A", Negated},
        {"B", Normal}
    }
};

SOP sop = {{term1,term2,term3}};

pin A = {"A",Input,13}; 
pin B = {"B", Input, 14};
pin Y = {"Y", Output, 0}; // Note zero here does not imply that the output pin has ANY capacitance.

Gate NAND2 = build_gate(
    sop,
    {A, B, Y},
    {
        {"X1", 14, {{14,13}, {15,16}}},
        {"X2", 15, {{14,13}, {15,16}}}
    }
);

int main(){
    std::cout << NAND2.pins[0].name <<std::endl; // Accessing
    std::cout << NAND2.function.SOP_Output.size() << std::endl; // Accessing Size
    return 0;
}