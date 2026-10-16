#include "gates.hpp"
#include <stdexcept>
#include "sop_eval.hpp"
#include <set>
std::vector<std::vector<bool>> input_creation(const Gate& g){
    int n = 0;
    for (const pin& p : g.pins) {
        if (p.direction == pin::pinDirections::Input) {
            n = n+1;
        }
    }
    std::vector<std::vector<bool>> inputs;
    for (int r = 0; r<int(std::pow(2.0,n));r++){
        std::vector<bool> row;
        for (int k = 0; k<n; k++){
            row.push_back((r>>k)&1);
        }
        inputs.push_back(row);  
    }
    return inputs;
}

void print_gate(const Gate& g){
    using namespace std;
    cout<<g.gate_name<<endl;
    cout<<"--PIN CONFIGURATIONS--"<<endl;
    for (const pin& p : g.pins) {
        cout<<"Pin name: "<<p.name<<endl;
        cout<<"Pin Direction: " << (p.direction == pin::pinDirections::Input ? "Input" : "Output") << endl;
        cout<<"Pin capacitance: " <<p.capacitance<<endl;
        }
    cout<<"--SOP CONFIGURATION--"<<endl;
    cout<<(g.function.Inverted_Flag == SOP::InvertedFlag::Normal ? "Not inverted" : "Inverted" )<< endl;
    int j = 0;
    for (const Term &t: g.function.Terms){
        j = j + 1;
        cout<<"Details for Term "<<j<<endl;
        for (const Literal &l: t.Literals){
            cout<<"Literal name: " << l.literal_name << endl;
            cout<<(l.polarity == Literal::LiteralPolarity::Normal ? "Normal Literal" : "Negated Literal") << endl;
            cout<<"Pin index: " << l.pinIndex << endl; 
        }
    }
    cout<<"--VARIANT DETAILS--"<<endl;
    for (const Variant &v : g.Variants){
        cout<< "Variant Name: " << v.variant_name << endl;
        cout<< "Area: " << v.area << endl;
        for(const auto& arc : v.ArcMap){
            cout<<"Input pin name: " << arc.first<<endl;
            cout<<"Rising delay: " << arc.second.rising_delay << " Falling delay: " << arc.second.falling_delay << endl;
        }
    }
    
}

std::map<std::string, std::vector<bool>> expected_tables(){
    std::map<std::string, std::vector<bool>> t;
    t["INV"]   = {true,  false};
    t["NAND2"] = {true,  true,  true,  false};
    t["NOR2"]  = {true,  false, false, false};
    t["AND2"]  = {false, false, false, true};
    t["OR2"]   = {false, true,  true,  true};
    t["NAND3"] = {true,  true,  true,  true,  true,  true,  true,  false};
    t["AOI21"] = {true,  true,  true,  false, false, false, false, false};
    return t;
}

int check_gate(const Gate& g, const std::map<std::string, std::vector<bool>>& expected){
    auto it = expected.find(g.gate_name);
    if (it == expected.end()) {
        std::cerr << "FAIL: no expected table for gate " << g.gate_name << std::endl;
        return 1;
    }
    const std::vector<bool>& want = it->second;
    std::vector<std::vector<bool>> rows = input_creation(g);
    if (rows.size() != want.size()) {
        std::cerr << "FAIL: " << g.gate_name << " has " << rows.size()
                  << " input rows but the expected table has " << want.size() << std::endl;
        return 1;
    }
    int problems = 0;
    for (size_t r = 0; r < rows.size(); r++){
        bool got = sop_eval(g.function, rows[r]);
        if (got != want[r]) {
            std::cerr << "FAIL: " << g.gate_name << " row " << r
                      << " expected " << want[r] << " got " << got << std::endl;
            problems = problems + 1;
        }
    }
    return problems;
}
 

int main(int argc, char** argv){
    bool verbose = (argc > 1 && std::string(argv[1]) == "-v");
 
    try {
        std::vector<Gate> library = build_library();
        const std::map<std::string, std::vector<bool>> expected = expected_tables();
        int problems = 0;

        if (library.size() != expected.size()) {
            std::cerr << "FAIL: library has " << library.size()
                      << " gates but " << expected.size() << " were expected" << std::endl;
            problems = problems + 1;
        }
 
        std::set<std::string> names;
        for (const Gate& g : library) {
            if (!names.insert(g.gate_name).second) {
                std::cerr << "FAIL: duplicate gate name " << g.gate_name << std::endl;
                problems = problems + 1;
            }
        }
 
        // The mapper cannot cover every circuit without these two.
        for (const std::string required : {"INV", "NAND2"}) {
            if (names.count(required) == 0) {
                std::cerr << "FAIL: required cell " << required << " is missing" << std::endl;
                problems = problems + 1;
            }
        }
 
        // Truth-table check for every gate.
        for (const Gate& g : library) {
            if (verbose) {
                print_gate(g);
            }
            problems = problems + check_gate(g, expected);
        }
 
        if (problems > 0) {
            std::cerr << problems << " problem(s) found" << std::endl;
            return 1;
        }
        std::cout << "All " << library.size() << " gates passed" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}