#pragma once 
#include <iostream>
#include <string>
#include <map>
#include <vector>

enum pinDirections{Input=0, Output=1}; // Input = 0, Output = 1

struct pin{ 
    std::string name;
    pinDirections direction;
    int capacitance; 
};

struct delay{int rising_delay; int falling_delay;};

struct Variant{
    std::string variant_name;
    int area;
    std::map<std::string, delay> ArcMap; // Destination, rising_delay, falling_delay
};
enum LiteralPolarity{Normal,Negated};
struct Literal{
    std::string literal_name;
    LiteralPolarity polarity;

};

struct Term{std::vector<Literal> Terms;};

struct SOP{std::vector<Term> SOP_Output;};


struct Gate{
    SOP function;
    std::vector<pin> pins;
    std::vector<Variant> Variants;

    std::map<std::string,std::string> arc_topology;
};

