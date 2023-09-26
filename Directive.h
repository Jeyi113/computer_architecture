#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>

using namespace std;

map<string, vector<string>> opMap()
{
    // make a map for opcode
    map<string, vector<string>> m;
    m["addu"] = vector<string>{"0","0x21"};
    m["and"] = vector<string>{"0","0x24"};
    m["jr"] = vector<string>{"0","8"};
    m["nor"] = vector<string>{"0","0x27"};
    m["or"] = vector<string>{"0","0x25"};
    m["sltu"] = vector<string>{"0","0x2b"};
    m["sll"] = vector<string>{"0","0"};
    m["srl"] = vector<string>{"0","2"};
    m["subu"] = vector<string>{"0","0x23"};
    m["addiu"] = vector<string>{"9"};
    m["andi"] = vector<string>{"0xc"};
    m["beq"] = vector<string>{"4"};
    m["bne"] = vector<string>{"5"};
    m["lui"] = vector<string>{"0xf"};
    m["lw"] = vector<string>{"0x23"};
    m["lb"] = vector<string>{"0x20"};
    m["ori"] = vector<string>{"0xd"};
    m["sltiu"] = vector<string>{"0xb"};
    m["sw"] = vector<string>{"0x2b"};
    m["sb"] = vector<string>{"0x28"};
    m["j"] = vector<string>{"2"};
    m["jal"] = vector<string>{"3"};

    return m;
}

bool isR(string fstElmt, map<string, vector<string>> ma) // find op+func (or op) through map
{
    if (ma[fstElmt].size() > 1){ return true; }
    else { return false; }
}

class Directive
{
    protected :
        string lb; // label
        string di; // directive
        string val; // value (for word)
        string tp; // opcode (for instruction)
        vector<string> inst; // value (for instruction)
        long long adrs; // address : indicates the first element's address
        int nn = 0; // not necessary : 1 if the content is just a word to describe the section, 0 it the content is crucial data.
    public : 
        Directive();
        Directive(string dorl, long long address, int cn);
        string getLabel();
        string getDirect();
        string getVal();
        int getNum();
        long long getAdrs();
        vector<string> getInst();
        virtual string getType(){return tp;}
};

class Words : public Directive
{
    public :
        Words();
        Words(string label, string value, long long address);
        long long getAdrs();
        string getVal();
};

class Instruction : public Directive
{
    private :
        int op;
        map<string, vector<string>> theMap;
    public :
        Instruction();
        Instruction(string label, vector<string> instruction, long long address);
        long long getAdrs();
        int getOp();
        string getType();
        vector<string> getInst();
        map<string,vector<string>> getMap();
};

//Base 1 : Directive
Directive::Directive(){}
Directive::Directive(string dorl, long long address, int cn)
{
    nn = 1;
    adrs = address;
    if(cn == 0){ di = dorl; lb = " "; }
    else if (cn == 1){ di = " "; lb = dorl;}
}
string Directive::getLabel(){return lb;}
string Directive::getDirect(){return di;}
string Directive::getVal(){return val;}
long long Directive::getAdrs(){return adrs;}
int Directive::getNum(){return nn;}
vector<string> Directive::getInst(){return inst;}

//Child 1 : Words
Words::Words(){}
Words::Words(string label, string value, long long address) // num 1 : starts with label, num 0 : no starts with label
{
    lb = label;
    di = ".word";
    val = value;
    adrs = address;
}
long long Words::getAdrs(){return adrs;}

//Child 2 : Instruction
Instruction::Instruction(){theMap = opMap();}
Instruction::Instruction(string label, vector<string> instruction, long long address)
{
    lb = label;
    di = ".text";
    inst = instruction;
    adrs = address;
        
    // operation type -> opcode, function (or etc)
    if(isR(instruction[0],opMap())){tp = "R";}
    else
    {
        if (instruction.size() == 2){tp = "J";}
        else{tp = "I";}
    }
}

long long Instruction::getAdrs(){return adrs;}
int Instruction::getOp(){return op;}
string Instruction::getType(){return tp;}
vector<string> Instruction::getInst(){return inst;}
map<string,vector<string>> Instruction::getMap(){return theMap;}