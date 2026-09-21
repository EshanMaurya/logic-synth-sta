#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>


using namespace std;

struct gate_basic{
    string output_name;
    string gate_type;
    vector<string> input_gates;
};

string whitespace_removal(string line){
    line.erase(remove(line.begin(),line.end(),' '), line.end());
    return line;
};

int main() {
    ifstream file("bench/c17.bench");
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
    cout << "INPUTS: ";
    for (auto& s : INPUTS) cout << s << " ";
    cout << endl;

    cout << "OUTPUTS: ";
    for (auto& s : OUTPUTS) cout << s << " ";
    cout << endl;

    for (auto& g : GATES) {
        cout << g.output_name << " = " << g.gate_type << "(";
        for (auto& in : g.input_gates) cout << in << " ";
        cout << ")" << endl;
    }    
    return 0;
}