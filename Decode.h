#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <bitset>

using namespace std;

map <string,string> TheMap()
{
    map<string,string> m;
    m.insert({"100001","0010"}); //addu
    m.insert({"100011","0110"}); //subu
    m.insert({"100100","0000"}); //and
    m.insert({"100101","0001"}); //or
    m.insert({"101011","0111"}); //sltu
    m.insert({"100111","1100"}); //nor
    return m;
}

class Decode
{
    protected:
        string type;
        string bits; // bit ver info
        int address;
    public:
        Decode();
        string get_tp();
        string get_bits();
        int get_adrs();
};
Decode::Decode(){}
string Decode::get_tp(){return type;}
string Decode::get_bits(){return bits;}
int Decode::get_adrs(){return address;}

class Words : public Decode
{
    private:
        string B1;
        string B2;
        string B3;
        string B4; // 1Byte actually
    public:
        Words();
        Words(bitset<32> b, int adrs);
        string get_valB1();
        string get_valB2();
        string get_valB3();
        string get_valB4();
};
Words::Words(){}
Words::Words(bitset<32> b, int adrs)
{
    bits = b.to_string(); address = adrs;

    // Segmentation of each byte
    B1 = bits.substr(0,8);
    B2 = bits.substr(8,8);
    B3 = bits.substr(16,8);
    B4 = bits.substr(24,8);
}
string Words::get_valB1(){return B1;}
string Words::get_valB2(){return B2;}
string Words::get_valB3(){return B3;}
string Words::get_valB4(){return B4;}

class Instructions : public Decode
{
    private:
        map<string,int> rm; // stores the register info
        string ALUOP;
        string ALUctrl;
        string fn;
        string op;
        int RegDst=0;
        int ALUScr=0;
        int Brch=0;
        int MemRead=0;
        int MemWrite=0;
        int RegWrite=0;
        int MemtoReg=0;
        int Jump=0;
        int disabled = 0;
    public:
        Instructions();
        Instructions(int x);
        Instructions(bitset<32> b, int adrs);
        map<string,int> getRm();
        string getALUOP();
        string getALUctrl();
        string getFn();
        string getOp();
        int getRegDst();
        int getALUScr();
        int getBrch();
        int getMemRead();
        int getMemWrite();
        int getRegWrite();
        int getMemtoReg();
        int getJump();
        int getDis();
};
Instructions::Instructions(){}
Instructions::Instructions(int x){disabled = 1;}
Instructions::Instructions(bitset<32> b, int adrs)
{
    bits = b.to_string(); address = adrs;
    /*Decode*/
    op = bits.substr(0,6);
    if (op == "000000") // R type
    {
        type = "R"; 
        fn = bits.substr(26);

        /*Control Signal*/
        ALUOP = "10"; 
        ALUctrl = TheMap()[fn];
        RegDst = 1; RegWrite = 1; MemtoReg = 1;
    
        /*Register read*/
        int rs, rt, rd, sh;
        rs = stoi(bits.substr(6,5),nullptr,2); rt = stoi(bits.substr(11,5),nullptr,2); rd = stoi(bits.substr(16,5),nullptr,2); sh = stoi(bits.substr(21,5),nullptr,2);
        
        if (fn == "001000"){rm.insert({"rs",rs});} //jr
        else if (fn == "000000"||fn == "000010"){rm.insert({"rt",rt}); rm.insert({"rd",rd}); rm.insert({"sh",sh});} //sll, srl
        else {rm.insert({"rs",rs}); rm.insert({"rt",rt}); rm.insert({"rd",rd});}
    }
    else if (op == "000010" || op == "000011") // J type
    {
        type = "J";
        string tg = bits.substr(6);
        /*Control Signal*/
        Jump = 1;
        /*JumpTarget read*/
        long long jt = stoll(tg,nullptr,2);
        rm.insert({"jt",jt});
    }
    else // I type
    {
        type = "I";
        string im = bits.substr(16);

        /*Control Signal*/
        if (op == "100011" || op == "101011" || op == "100000" || op == "101000") //lw(lb) or sw(sb)
        {
            ALUOP = "00"; ALUctrl = "0010"; ALUScr = 1;
            if (op == "100011" || op == "100000"){/*load*/MemRead = 1; RegWrite = 1; MemtoReg = 1;}
            else {/*store*/MemWrite = 1;}
        }
        else if (op == "000100" ||op =="000101"){ALUOP = "01"; ALUctrl = "0110"; Brch = 1;} // beq or bne
        else if (op == "001001"){ALUctrl = "0010";} // addiu
        else if (op == "001100"){ALUctrl = "0000";} // andi
        else if (op == "001101"){ALUctrl = "0001";} // ori
        else if (op == "001011"){ALUctrl = "0111";} // sltiu
        else{}
        /*Register read*/
        int rs, rt; bitset<16> bim(im); int imm = static_cast<short>(bim.to_ulong());
        rs = stoi(bits.substr(6,5),nullptr,2); rt = stoi(bits.substr(11,5),nullptr,2);
        rm.insert({"rs", rs});
        rm.insert({"rt", rt});
        if (op == "001111" && imm == 0){im = bits.substr(11); imm = stoul(im,nullptr,2);} // lui
        rm.insert({"imm", imm});
    }
}
map<string,int> Instructions::getRm(){return rm;}
string Instructions::getALUOP(){return ALUOP;}
string Instructions::getALUctrl(){return ALUctrl;}
string Instructions::getFn(){return fn;}
string Instructions::getOp(){return op;}
int Instructions::getRegDst(){return RegDst;}
int Instructions::getALUScr(){return ALUScr;}
int Instructions::getBrch(){return Brch;}
int Instructions::getMemRead(){return MemRead;}
int Instructions::getMemWrite(){return MemWrite;}
int Instructions::getRegWrite(){return RegWrite;}
int Instructions::getMemtoReg(){return MemtoReg;}
int Instructions::getJump(){return Jump;}
int Instructions::getDis(){return disabled;}



