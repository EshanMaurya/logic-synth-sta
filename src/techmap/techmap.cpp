#include <iostream>
#include <vector>
#include "gates.hpp"
#include "techmap.hpp"
#include "decompose.hpp"
#include <algorithm>
#include <string>
#include <stdexcept>

static int count_inputs(const Gate &g)
{
    int num_inputs = static_cast<int>(std::count_if(g.pins.begin(), g.pins.end(),
                                                    [](const pin &p)
                                                    { return p.direction == pin::pinDirections::Input; }));
    return num_inputs;
}

std::vector<pattern> build_patterns()
{
    std::vector<Gate> Gates = build_library();
    int i = 0;
    std::vector<pattern> patternvector;
    for (const Gate &g : Gates)
    {
        int num_inputs = count_inputs(g);
        SubjectGraph graph = decompose(g.function, num_inputs);
        pattern p;
        p.area = g.Variants.at(0).area; // Using any variant for now, will fix later
        p.gateIndex = i;
        p.patternGraph = graph;
        p.patternName = g.gate_name;
        p.num_inputs = num_inputs;
        i += 1;
        patternvector.push_back(p);
    }
    return patternvector;
}

static bool match_here(const SubjectGraph &patternGraph, const int &patternNode, const SubjectGraph &circuitGraph, const int &circuitNode, std::vector<int> &bindings)
{
    const node &pnode = patternGraph.VectorNode.at(patternNode);
    node::type ptype = pnode.NodeType;

    const node &cnode = circuitGraph.VectorNode.at(circuitNode);
    node::type ctype = cnode.NodeType;

    if (ptype == node::type::PI)
    {
        int pinNumber = pnode.pinIndex;
        if (bindings.at(pinNumber) == (-1))
        {
            bindings.at(pinNumber) = circuitNode;
            return true;
        }
        else
            return bindings.at(pinNumber) == circuitNode;
    }
    else if (ptype == node::type::INV)
    {
        if (ctype != ptype)
        {
            return false;
        }
        return match_here(patternGraph, pnode.Inputs.at(0), circuitGraph, cnode.Inputs.at(0), bindings); // Recursively
    }
    else if (ptype == node::type::NAND)
    {
        if (ctype != node::type::NAND)
        {
            return false;
        }
        std::vector<int> bindingscopy = bindings;
        if (match_here(patternGraph, pnode.Inputs.at(0), circuitGraph, cnode.Inputs.at(0), bindings) && match_here(patternGraph, pnode.Inputs.at(1), circuitGraph, cnode.Inputs.at(1), bindings))
        { // Straight order
            return true;
        }
        else
            bindings = bindingscopy;

        if (match_here(patternGraph, pnode.Inputs.at(0), circuitGraph, cnode.Inputs.at(1), bindings) && match_here(patternGraph, pnode.Inputs.at(1), circuitGraph, cnode.Inputs.at(0), bindings))
        { // Crossed order
            return true;
        }
        else
        {
            bindings = bindingscopy;
            return false;
        }
    }
    else
    {
        throw std::invalid_argument("Unknown node type");
    }
}

std::vector<match> find_matches(const SubjectGraph &circuitGraph, int circuitNode, const std::vector<pattern> &patterns)
{
    std::vector<match> result;
    int i = 0;
    for (const pattern &p : patterns)
    {
        std::vector<int> bindings(p.num_inputs, -1);
        if (match_here(p.patternGraph, p.patternGraph.root, circuitGraph, circuitNode, bindings))
        {
            match m;
            m.patternIndex = i;
            m.bindings = bindings;
            result.push_back(m);
        }
        i += 1;
    }
    return result;
}

std::vector<tmap_result> dp_cover(const SubjectGraph &circuit, const std::vector<pattern> &patterns)
{
    std::vector<tmap_result> resultvector;
    int k = 0; // node counter
    for (const node &n : circuit.VectorNode)
    {
        tmap_result result;
        if (n.NodeType == node::type::PI)
        {
            result.bestCost = 0;
        }
        else
        {
            std::vector<match> matches = find_matches(circuit, k, patterns);
            if (matches.size() == 0)
            {
                throw std::runtime_error("No matches found at node " + std::to_string(k));
            }

            for (const match &m : matches)
            {
                const pattern &p = patterns.at(m.patternIndex);
                int total = p.area;

                for (const int &b : m.bindings)
                {
                    total += resultvector.at(b).bestCost;
                }
                if (total < result.bestCost)
                {
                    result.bestCost = total;
                    result.bestMatch = m;
                }
            }
        }
        k += 1;
        resultvector.push_back(result);
    }
    return resultvector;
}

static void extract_node(const std::vector<tmap_result> &result, int nodeIndex, std::vector<bool> &visited, std::vector<mapped_cell> &output)
{

    if (visited.at(nodeIndex) == true)
    {
        return;
    }
    else
        visited.at(nodeIndex) = true;
    if (result.at(nodeIndex).bestMatch.patternIndex == -1)
    {
        return; // Nothing to place, it's a PI
    }
    else
    {
        for (int n : result.at(nodeIndex).bestMatch.bindings)
        {
            extract_node(result, n, visited, output);
        }
        mapped_cell m;
        m.inputNodes = result.at(nodeIndex).bestMatch.bindings;
        m.patternIndex = result.at(nodeIndex).bestMatch.patternIndex;
        m.outputNode = nodeIndex;
        output.push_back(m);
    }
}

std::vector<mapped_cell> top_down_extractor(const std::vector<tmap_result> &resultTable, int rootNode)
{
    std::vector<bool> visited(resultTable.size(), false);
    std::vector<mapped_cell> output;
    extract_node(resultTable, rootNode, visited, output);
    return output;
}

SOP cover_to_sop(const std::vector<lss::Cube> &cover, const std::vector<std::string> &var_names)
{
    if (cover.size() == 0)
    {
        throw std::invalid_argument("cover_to_sop: empty cover (constant 0)");
    }

    SOP result;
    int k = 0; // for error messages
    for (const lss::Cube &c : cover)
    {
        if (c.num_vars() != var_names.size())
        {
            throw std::invalid_argument("cover_to_sop: cube " + std::to_string(k) + " has " +
                                        std::to_string(c.num_vars()) + " variables but " +
                                        std::to_string(var_names.size()) + " names were given");
        }
        Term t;
        for (std::size_t i = 0; i < c.num_vars(); i++)
        {
            lss::Lit state = c.get(i);
            if (state == lss::Lit::DontCare)
            {
                continue;
            }
            else if (state == lss::Lit::One)
            {
                Literal l;
                l.literal_name = var_names.at(i);
                l.polarity = Literal::LiteralPolarity::Normal;
                l.pinIndex = static_cast<int>(i);
                t.Literals.push_back(l);
            }
            else if (state == lss::Lit::Zero)
            {
                Literal l;
                l.literal_name = var_names.at(i);
                l.polarity = Literal::LiteralPolarity::Negated;
                l.pinIndex = static_cast<int>(i);
                t.Literals.push_back(l);
            }
            else
                throw std::invalid_argument("cover_to_sop: cube " + std::to_string(k) + " has an invalid literal");
        }

        if (t.Literals.size() == 0)
        {
            throw std::invalid_argument("cover_to_sop: cube " + std::to_string(k) + " has no literals (constant 1)");
        }
        result.Terms.push_back(t);
        k += 1;
    }

    result.Inverted_Flag = SOP::InvertedFlag::Normal;
    return result;
}