#pragma once
#include "cell_library.hpp"

// A simplified struct to describe variant's numbers before the Gate is built.

struct VariantInput {
    std::string variant_name;
    int area;
    std::vector<delay> delays;
};

// Derives arc_topology automatically: every Input pin -> the single Output pin. Independent of variant!
std::map<std::string, std::string> build_arc_topology(const std::vector<pin>& pins) {
    std::map<std::string, std::string> topology;
    std::string output_pin_name;

    // For the output pin
    for (const pin& p : pins) {
        if (p.direction == Output) {
            output_pin_name = p.name;
        }
    }

    // For the input pin
    for (const pin& p : pins) {
        if (p.direction == Input) {
            topology[p.name] = output_pin_name;
        }
    }

    return topology;
}

// Builds Variant's ArcMap.
Variant build_variant(const std::vector<pin>& pins, const VariantInput& input) {
    std::map<std::string, delay> arc_map;

    std::vector<std::string> input_pin_names;
    for (const pin& p : pins) {
        if (p.direction == Input) {
            input_pin_names.push_back(p.name);
        }
    }

    // Check: input.delays must line up 1-to-1 with input pins.
    if (input_pin_names.size() != input.delays.size()) {
        std::cout << "Mismatch: " << input.variant_name
                  << " has " << input.delays.size() << " delay entries but "
                  << input_pin_names.size() << " input pins." << std::endl;
    }

    for (size_t i = 0; i < input_pin_names.size(); i++) {
        arc_map[input_pin_names[i]] = input.delays[i];
    }

    return Variant{input.variant_name, input.area, arc_map};
}

// Top-level builder
Gate build_gate(const SOP& function, const std::vector<pin>& pins,
                 const std::vector<VariantInput>& variant_inputs) {
    std::vector<Variant> variants;
    for (const VariantInput& vi : variant_inputs) {
        variants.push_back(build_variant(pins, vi)); // pushback being analogous to .append()
    }

    return Gate{function, pins, variants, build_arc_topology(pins)};
}
