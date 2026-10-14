#pragma once 
#include <iostream>
#include <string>
#include <map>
#include <vector>



struct pin{ 
    enum class pinDirections{Input=0, Output=1}; // Input = 0, Output = 1
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

struct Literal{
    enum class LiteralPolarity{Normal,Negated};
    std::string literal_name;
    LiteralPolarity polarity;
    int pinIndex = -1;
};

struct Term{std::vector<Literal> Literals;};

struct SOP{
    enum class InvertedFlag{Normal,Inverted};
    std::vector<Term> Terms;
    InvertedFlag Inverted_Flag;};


struct Gate{
    std::string gate_name;
    SOP function;
    std::vector<pin> pins;
    std::vector<Variant> Variants;
    std::map<std::string,std::string> arc_topology;
};

