#include "gates.hpp"
#include "build_gate.hpp"

std::vector<Gate> build_library(){
    std::vector<Gate> Gates;
    // NAND2
    {
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
                {"X1", 12, {{12,10}, {13,11}}},
                {"X2", 18, {{9,8}, {10,9}}}
            },
            "NAND2"
        );
        Gates.push_back(NAND2);

    // INV
    }

        {
        Term term1 = {
            {
                {"A", Literal::LiteralPolarity::Normal}
            }
        };
        SOP sop = {
            {term1}, 
            SOP::InvertedFlag::Inverted
        };
        pin A = {"A",pin::pinDirections::Input,13}; 
        pin Y = {"Y", pin::pinDirections::Output, 0}; // Note zero here does not imply that the output pin has ANY capacitance.

        Gate INV = build_gate(
            sop,
            {A,Y},
            {
                {"X1", 8, {{8,7}}},
                {"X2", 12, {{6,5}}}
            },
            "INV"
        );
        Gates.push_back(INV);
    }
        // NAND3
    {
        Term term1 = {{
            {"A", Literal::LiteralPolarity::Normal},
            {"B", Literal::LiteralPolarity::Normal},
            {"C", Literal::LiteralPolarity::Normal}
        }};
        SOP sop = { {term1}, SOP::InvertedFlag::Inverted };
        pin A = {"A", pin::pinDirections::Input, 10};
        pin B = {"B", pin::pinDirections::Input, 10};
        pin C = {"C", pin::pinDirections::Input, 10};
        pin Y = {"Y", pin::pinDirections::Output, 0};

        Gates.push_back(build_gate(
            sop, {A, B, C, Y},
            {
                {"X1", 16, {{16,13}, {17,14}, {18,15}}},
                {"X2", 24, {{12,10}, {13,11}, {14,12}}}
            },
            "NAND3"));
    }

    // NOR2
    {
        Term term1 = {{ {"A", Literal::LiteralPolarity::Normal} }};
        Term term2 = {{ {"B", Literal::LiteralPolarity::Normal} }};
        SOP sop = { {term1, term2}, SOP::InvertedFlag::Inverted };
        pin A = {"A", pin::pinDirections::Input, 10};
        pin B = {"B", pin::pinDirections::Input, 10};
        pin Y = {"Y", pin::pinDirections::Output, 0};

        Gates.push_back(build_gate(
            sop, {A, B, Y},
            {
                {"X1", 14, {{14,12}, {15,13}}},
                {"X2", 21, {{11,9},  {12,10}}}
            },
            "NOR2"));
    }

    // AND2
    {
        Term term1 = {{
            {"A", Literal::LiteralPolarity::Normal},
            {"B", Literal::LiteralPolarity::Normal}
        }};
        SOP sop = { {term1}, SOP::InvertedFlag::Normal };
        pin A = {"A", pin::pinDirections::Input, 9};
        pin B = {"B", pin::pinDirections::Input, 9};
        pin Y = {"Y", pin::pinDirections::Output, 0};

        Gates.push_back(build_gate(
            sop, {A, B, Y},
            {
                {"X1", 16, {{20,18}, {21,19}}},
                {"X2", 24, {{15,14}, {16,15}}}
            },
            "AND2"));
    }

    // OR2
    {
        Term term1 = {{ {"A", Literal::LiteralPolarity::Normal} }};
        Term term2 = {{ {"B", Literal::LiteralPolarity::Normal} }};
        SOP sop = { {term1, term2}, SOP::InvertedFlag::Normal };
        pin A = {"A", pin::pinDirections::Input, 9};
        pin B = {"B", pin::pinDirections::Input, 9};
        pin Y = {"Y", pin::pinDirections::Output, 0};

        Gates.push_back(build_gate(
            sop, {A, B, Y},
            {
                {"X1", 18, {{22,20}, {23,21}}},
                {"X2", 27, {{17,15}, {18,16}}}
            },
            "OR2"));
    }

    // AOI21 : (A*B + C)'
    {
        Term term1 = {{
            {"A", Literal::LiteralPolarity::Normal},
            {"B", Literal::LiteralPolarity::Normal}
        }};
        Term term2 = {{ {"C", Literal::LiteralPolarity::Normal} }};
        SOP sop = { {term1, term2}, SOP::InvertedFlag::Inverted };
        pin A = {"A", pin::pinDirections::Input, 11};
        pin B = {"B", pin::pinDirections::Input, 11};
        pin C = {"C", pin::pinDirections::Input, 10};
        pin Y = {"Y", pin::pinDirections::Output, 0};

        Gates.push_back(build_gate(
            sop, {A, B, C, Y},
            {
                {"X1", 18, {{16,14}, {17,15}, {14,12}}},
                {"X2", 27, {{12,11}, {13,12}, {11,9}}}
            },
            "AOI21"));
    }
    return Gates;
}