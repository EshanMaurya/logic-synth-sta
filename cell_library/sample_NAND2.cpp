#include "cell_library.hpp"

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

Variant X1{
    "X1",
    14,
    {
        {"A", {14,13}},
        {"B", {15,16}}
    }
};


Variant X2{
    "X2",
    15,
    {
        {"A", {14,13}},
        {"B", {15,16}}
    }
};

Gate NAND2{
    sop,
    {A,B,Y},
    {X1,X2},
    {
        {"A","Y"},
        {"B","Y"}
    }
};

int main(){
    std::cout << NAND2.pins[0].name <<std::endl; // Accessing pin names
    std::cout << NAND2.function.SOP_Output.size() << std::endl; // Accessing Size
    return 0;
}