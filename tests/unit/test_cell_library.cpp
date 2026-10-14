#include "gates.hpp"
#include <stdexcept>



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

int main(){
    try {
        std::vector<Gate> gates = build_library();
        int i = 0;
        for (const auto& gate : gates){
            std::cout<<"Gate No." <<i<<std::endl;
            print_gate(gate);
            i = i+1;
        }
    }
    

    catch( const std::exception&e){
        std::cerr<<e.what()<<std::endl;
    }
}