#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <cctype>
#include <cstdlib>

using namespace std;

struct gate_basic{
    string output_name;
    string gate_type;
    vector<string> input_gates;
};

enum GateType {
        PI,AND, OR, NOR, XOR, XNOR, NOT, BUFF, NAND
};

struct Node{
    string name;
    GateType gate_type;
    std::vector<int> fanin;
    std::vector<int> fanout;
    bool is_output;
};

string whitespace_removal(string line){
    line.erase(remove(line.begin(),line.end(),' '), line.end());
    return line;
};

vector<Node> parse_bench_file(string path, bool verbose){
    ifstream file(path);
    string line;
    vector<string> INPUTS;
    vector<string> OUTPUTS;
    vector<gate_basic> GATES;
    while (getline(file, line)) {
        string line_w = whitespace_removal(line);
        if (line_w == ""){
            continue;
        }
        else if (line_w.find("#") != string::npos){
            continue;
        }
        else if (line_w.find("INPUT(") != string::npos){
            size_t a  = line_w.find("INPUT(");
            size_t b = line_w.find(")");
            string input = line_w.substr(a+6,b-(a + 6));
            INPUTS.push_back(input);
        }
        else if (line_w.find("OUTPUT(") != string::npos){
            size_t a  = line_w.find("OUTPUT(");
            size_t b = line_w.find(")");
            string output = line_w.substr(a+7,b-(a + 7));
            OUTPUTS.push_back(output);
        }
        else if (line_w.find("=") != string::npos){
            size_t a = line_w.find("=");
            size_t b = line_w.find("(");
            size_t closing_paren = line_w.find(")");
            string output = line_w.substr(0,a);
            string gate = line_w.substr(a+1,b-a-1);
            vector<string> inputs;
            int i = 0;
            int d = b+1;
            while (i==0){
                size_t c = line_w.find(",",d);
                if (c != string::npos && c<closing_paren){
                    string input_current = line_w.substr(d,c-d);
                    inputs.push_back(input_current);
                    d = c+1;
                }
                else{
                    string input_current = line_w.substr(d, closing_paren-d);
                    inputs.push_back(input_current);
                    i=1;
                }
            }
            gate_basic logic_gate = {output, gate, inputs};
            GATES.push_back(logic_gate);
        }
    };
    std::unordered_map<string, int> ID;
    for (int idx = 0; idx<INPUTS.size(); idx++){
        ID.insert({INPUTS[idx], idx});
    }
    int id_counter = 0;
    for (int idx=INPUTS.size(); idx<INPUTS.size()+ GATES.size(); idx++){
        ID.insert({GATES[id_counter].output_name, idx});
        id_counter+=1;
    }

    // Creation of PI gate_type Nodes
    vector<Node> nodes;
    for (int idx=0; idx<INPUTS.size(); idx++){
        Node inputgates = {INPUTS[idx], PI, {}, {}, false};
        nodes.push_back(inputgates);
    }

    // Gates MAP
    std::unordered_map<string, GateType> gate_type_lookup = {
        {"AND", AND},
        {"OR", OR},
        {"NOR", NOR},
        {"XOR", XOR},
        {"XNOR", XNOR},
        {"NOT", NOT},
        {"BUFF", BUFF},
        {"NAND", NAND}
    };

    for (int idx=0; idx<GATES.size(); idx++){
        vector<int> fanin_ids;
        for (int gate_idx=0; gate_idx< GATES[idx].input_gates.size(); gate_idx++){
            fanin_ids.push_back(ID[GATES[idx].input_gates[gate_idx]]);
        }
        bool is_output;
        if (std::find(OUTPUTS.begin(), OUTPUTS.end(), GATES[idx].output_name) != OUTPUTS.end()){
            is_output = true;
        }
        else{
            is_output = false;
        }
        string gate_name = GATES[idx].gate_type;
        std::transform(gate_name.begin(), gate_name.end(), gate_name.begin(), [](unsigned char c){
            return std::toupper(c);
        }); // Converting to uppercase
        Node temporary;
        // Checking for errors, incase gate_type is not within Enums
        if (gate_type_lookup.find(gate_name) == gate_type_lookup.end()){
            cerr<<"Error found! Unrecognized gate type"<<endl;
            exit(1); // Stop the program
        }else{
            temporary = {GATES[idx].output_name, gate_type_lookup[gate_name], fanin_ids, {}, is_output};
        }
        nodes.push_back(temporary);
    }
    for (int idx=0; idx<nodes.size();idx++){
        for (int j = 0; j<nodes[idx].fanin.size(); j++){
            nodes[ nodes[idx].fanin[j] ].fanout.push_back(idx);
        }
    }
    if (verbose == true){
        for (auto& n : nodes) {
            cout << n.name << " (type=" << n.gate_type << ", output=" << n.is_output << ") fanin: ";
            for (auto f : n.fanin) cout << f << " ";
            cout << "| fanout: ";
            for (auto f : n.fanout) cout << f << " ";
            cout << endl;
    }
    }
    return nodes;

}

int main(){
    vector<Node> nodes =  parse_bench_file("bench/c432.bench", true); 
}