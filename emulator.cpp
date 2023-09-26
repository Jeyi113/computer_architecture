#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <bitset>
#include <unistd.h>
#include "Decode.h"

using namespace std;

//Initial States
int PC = 0x400000;
map<string,long> R = {{"R0",0},{"R1", 0},{"R2",0},{"R3",0},{"R4",0},{"R5",0},{"R6",0},{"R7",0},{"R8",0},{"R9",0},{"R10",0},{"R11",0},{"R12",0},{"R13",0},{"R14",0},{"R15",0},{"R16",0},{"R17",0},{"R18",0},{"R19",0},{"R20",0},{"R21",0},{"R22",0},{"R23",0},{"R24",0},{"R25",0},{"R26",0},{"R27",0},{"R28",0},{"R29",0},{"R30",0},{"R31",0}};
vector <pair<Instructions,int>> tSection;
vector <pair<Words,int>> dSection;
Instructions currentInst;

vector<bitset<32>> FileIn(string file)
{
    ifstream read(file);
    string content;
    vector<bitset<32>> fBits;
    //1. open the file, store each contenst (convert hex to bit)
    if (read.is_open())
    {
        while(read.peek() != EOF)
        {
            getline(read, content);
            content.erase(0,2); // erase "0x"
            long dcontent = stol(content, nullptr, 16);
            bitset<32> bits(dcontent);
            fBits.push_back(bits);
        }
        read.close();
    }
    else {cout << "Cannot open the file" << endl;}
    return fBits;
}

long FindValue(int adrs)
{
    for (int i = 0; i < dSection.size(); i++)
    {if (dSection[i].first.get_adrs() == adrs){return stol(dSection[i].first.get_bits(),nullptr,2);}}
    return -1;
}

int FindValueB(int adrs)
{
    for (int i = 0; i < dSection.size(); i++)
    {
        if (dSection[i].first.get_adrs() == adrs){return stoi(dSection[i].first.get_valB1(),nullptr,2);} 
        else if (dSection[i].first.get_adrs() + 1 == adrs){return stoi(dSection[i].first.get_valB2(),nullptr,2);}
        else if (dSection[i].first.get_adrs() + 2 == adrs){return stoi(dSection[i].first.get_valB3(),nullptr,2);}
        else if (dSection[i].first.get_adrs() + 3 == adrs){return stoi(dSection[i].first.get_valB4(),nullptr,2);}
    }
    return -1;
}

long returnDatas(unsigned long adrs)
{
    for (int i = 0; i < dSection.size(); i++){if (dSection[i].first.get_adrs() == adrs){return stol(dSection[i].first.get_bits(),nullptr,2);}}
    for (int i = 0; i < tSection.size(); i++){if (tSection[i].first.get_adrs() == adrs){return stol(tSection[i].first.get_bits(),nullptr,2);}}
    return 0; // No matching
}

void SetWords(int adrs, bitset<32> bi)
{
    int cover = 0;
    Words w(bi,adrs);
    for (int i = 0; i < dSection.size(); i++)
    {
        if (dSection[i].first.get_adrs() == adrs){cover = 1; Words w(bi,adrs); dSection[i] = {w,adrs};}
    }
    if (cover != 1){dSection.push_back({w,adrs});} // add new memory
}

void SetWordsB(int adrs, bitset<8> bi)
{   
    int cover = 0;
    for (int i = 0; i < dSection.size(); i++)
    {   
        string val;
        string b1 = dSection[i].first.get_valB1();
        string b2 = dSection[i].first.get_valB2();
        string b3 = dSection[i].first.get_valB3();
        string b4 = dSection[i].first.get_valB4();
        if (dSection[i].first.get_adrs() == adrs){cover = 1; b1 = bi.to_string(); val = b1+b2+b3+b4; bitset<32> br(val); Words w(br,adrs); dSection[i] = {w,adrs};}
        else if (dSection[i].first.get_adrs() + 1 == adrs){cover = 1; b2 = bi.to_string(); val = b1+b2+b3+b4; bitset<32> br(val); Words w(br,adrs-1); dSection[i] = {w,adrs-1};}
        else if (dSection[i].first.get_adrs() + 2 == adrs){cover = 1; b3 = bi.to_string(); val = b1+b2+b3+b4; bitset<32> br(val); Words w(br,adrs-2); dSection[i] = {w,adrs-2};}
        else if (dSection[i].first.get_adrs() + 3 == adrs){cover = 1; b4 = bi.to_string(); val = b1+b2+b3+b4; bitset<32> br(val); Words w(br,adrs-3); dSection[i] = {w,adrs-3};}
    }
    if (cover != 1){bitset<32> b(bi.to_ulong()); Words w(b,adrs); dSection.push_back({w,adrs});}
}

string RegNum(int rn){string reg = "R" + to_string(rn); return reg;}

void Execute()
{
    // 4. Execute the instruction
    /* Read the Instruction */
    string type = currentInst.get_tp();
    string operation = currentInst.getALUctrl(); 
    map<string,int> r = currentInst.getRm();
    if (stol(currentInst.get_bits(),nullptr,2)!=0)
    {
        if (type == "R") // RegWrite, ALUctrl
        {
            if(currentInst.getRegWrite() != 1){PC = R[RegNum(r["rs"])];} //jr
            else
            {
                string func = currentInst.getFn();

                if(operation == "0010"){R[RegNum(r["rd"])] = unsigned (int(R[RegNum(r["rs"])] + R[RegNum(r["rt"])]));} //addu
                if(operation == "0110"){R[RegNum(r["rd"])] = unsigned (int(R[RegNum(r["rs"])] - R[RegNum(r["rt"])]));} //subu
                if(operation == "0000"){R[RegNum(r["rd"])] = unsigned (int(R[RegNum(r["rs"])] & R[RegNum(r["rt"])]));} //and
                if(operation == "0001"){R[RegNum(r["rd"])] = unsigned (int(R[RegNum(r["rs"])] | R[RegNum(r["rt"])]));} //or
                if(operation == "0111"){if(unsigned (int(R[RegNum(r["rs"])])) < unsigned (int(R[RegNum(r["rt"])]))){R[RegNum(r["rd"])] = 1;} else{R[RegNum(r["rd"])] = 0;}} //sltu
                if(operation == "1100"){R[RegNum(r["rd"])] = unsigned (int(~((R[RegNum(r["rs"])] | R[RegNum(r["rt"])]))));} //nor            
            
                if(func == "000000"){R[RegNum(r["rd"])] = R[RegNum(r["rt"])] << r["sh"];} //sll
                if(func == "000010"){R[RegNum(r["rd"])] = R[RegNum(r["rt"])] >> r["sh"];} //srl
            }
        }
        else if (type == "I")
        {
            string opcode = currentInst.getOp();
            if(currentInst.getALUScr() == 1) //lw, sw
            {   
                if(opcode == "100011"){int address = r["imm"] + R[RegNum(r["rs"])]; long val = FindValue(address); R[RegNum(r["rt"])] = val;} //lw
                if(opcode == "100000"){int address = r["imm"] + R[RegNum(r["rs"])]; long val = FindValueB(address); R[RegNum(r["rt"])] = val;} //lb
                if(opcode == "101011")
                {   
                    int address = r["imm"] + R[RegNum(r["rs"])];
                    bitset<32> b(R[RegNum(r["rt"])]);
                    SetWords(address, b); 
                    
                } //sw
                if(opcode == "101000")
                {
                    int address = r["imm"] + R[RegNum(r["rs"])]; 
                    bitset<32> b(R[RegNum(r["rt"])]); 
                    bitset<8> b8(b.to_ulong());
                    SetWordsB(address, b8);
                } //sb
            }
            else if(currentInst.getBrch() == 1) //beq, bne
            {
                if(opcode == "000100")
                {int sub = R[RegNum(r["rt"])] - R[RegNum(r["rs"])]; if(sub == 0){PC += 4*r["imm"];}} //beq
                if(opcode == "000101")
                {int sub = R[RegNum(r["rt"])] - R[RegNum(r["rs"])]; if(sub != 0){PC += 4*r["imm"];}} //bne
            }
            else
            {
                if(operation == "0010"){R[RegNum(r["rt"])] = unsigned (int(R[RegNum(r["rs"])] + r["imm"]));} //addiu
                if(operation == "0000"){R[RegNum(r["rt"])] = R[RegNum(r["rs"])] & r["imm"];} //andi
                if(operation == "0001"){R[RegNum(r["rt"])] = R[RegNum(r["rs"])] | r["imm"];} //ori
                if(operation == "0111"){if(unsigned (int(R[RegNum(r["rs"])])) < unsigned (int(r["imm"]))){R[RegNum(r["rt"])] = 1;} else{R[RegNum(r["rt"])] = 0;}} //sltiu

                if(opcode == "001111"){R[RegNum(r["rt"])] = r["imm"] << 16;} //lui
            }
        } 
        else if (type == "J")
        {
            string opcode = currentInst.getOp();

            if(opcode == "000010")
            {PC = ((PC & 0xf0000000)|(r["jt"] << 2));} //J
            if(opcode == "000011"){R["R31"] = PC; PC = ((PC & 0xf0000000)|(r["jt"] << 2));} //JAL
        }
    }
}



int main(int argc, char* argv[])
{
    // 0. Check the commend input
    string filename;
    string adrs;
    long addr1, addr2; // memory addr range
    int iNum = 1; // instruction number to execute
    int m_op = 0, d_op = 0, n_op = 0;
    int opt;

    while ((opt = getopt(argc, argv, "m:dn:")) != -1)
    {
        switch(opt){
            case 'm':
                adrs = optarg;
                if (optarg == NULL || adrs.find(":") == string::npos){cout << "The range of address is required with flag -m." << endl; return 1;}
                else if (adrs.find("0x") == string::npos){cout << "The range of address should be the form of hexadecimal like '0xffffffff'." << endl; return 1;}
                m_op = 1;
                addr1 = stoul(adrs.substr(2,adrs.find(":")),nullptr,16);
                addr2 = stoul(adrs.substr(adrs.find(":")+3),nullptr,16); // remove "0x"

                //Exceptions
                if (addr1 > addr2){cout << "first address should be lower than second one." << endl; return 1;}
                if (addr1 < 0x00400000){cout << "The address is out of the range. The minimum is 0x00400000." << endl; return 1;}
                if ((addr1-0x00400000)%4 != 0 || abs(addr1-0x10000000)%4 != 0 || (addr2-0x00400000)%4 != 0 || abs(addr2-0x10000000)%4 != 0){cout << "The Range of address should be the unit of word." << endl; return 1;}
                break;
            case 'd':
                d_op = 1;
                break;
            case 'n':
                if (optarg == NULL){cout << "The number of instruction is required with flag -n." << endl; return 1;}
                else if (optarg < 0){cout << "The number of instruction must be positive integer number." << endl; return 1;}
                n_op = 1;
                iNum = atoi(optarg);
                break;
            case '?':
                cout << "unknown flag inserted." << endl;
                return 1;
            default:
                abort();
        }
    }

    if (optind < argc){filename = argv[optind];}
    else {cout << "There's no file provided." << endl; return 1;}

    // 1. Read the File
    vector<bitset<32>> datas = FileIn(filename);

    // 2. Analyze the contents
    /* Store to the memory */
    int textAdrs = 0x00400000;
    int wordAdrs = 0x10000000;
    //Read Header and set the segment size to allocate the memory
    int tSize = stoi(datas[0].to_string(),nullptr,2)/4;
    for (int i=2; i < tSize + 2; i++) // t
    {if(datas[i].to_ulong() != 0){Instructions t(datas[i], textAdrs); pair<Instructions,int> p = make_pair(t,textAdrs); textAdrs += 4; tSection.push_back(p);} }
    for (int i= 2 + tSize; i < datas.size(); i++) // d
    {Words w(datas[i],wordAdrs); pair<Words,int> p = make_pair(w,wordAdrs); wordAdrs += 4; dSection.push_back(p);}

    // 3. Fetch & Execute
    while (PC < 0x400000 + 4*tSize && iNum > 0)
    {   /*Fetch*/
        for (int i = 0; i<tSection.size(); i++){if(PC == tSection[i].second){currentInst = tSection[i].first;}}
        PC += 4;
        /*Execute*/
        Execute(); 
        if(n_op){iNum--;} // -n flag
        if(d_op) // -d flag
        {
            cout << "Current register values:" << endl;
            cout << "------------------------------------" << endl;
            cout << "PC: 0x" << hex << currentInst.get_adrs() << "\nRegisters:" << endl;
            for(int i = 0; i < R.size(); i++)
            {
                string reg = "R"+to_string(i); cout << reg << ": 0x";
                int convert = R[reg];
                cout << hex << convert << endl;
            }
            cout << "\n";
        }
    }
    if(d_op != 1) // w.o -d flag
    {
        cout << "Current register values:" << endl;
        cout << "------------------------------------" << endl;
        if (iNum != 0){cout << "PC: 0x" << hex << currentInst.get_adrs() << "\nRegisters:" << endl;}
        else {cout << "PC: 0x" << hex << PC << "\nRegisters:" << endl;}
        for(int i = 0; i < R.size(); i++)
        {string reg = "R"+to_string(i); cout << reg << ": 0x" << hex << R[reg] << endl;}
        cout << endl;
    }

    // -m flag
    if (m_op)
    {
        cout << "Memory content [0x" << hex << addr1 << "..0x" << addr2 <<"]:" << endl;
        cout << "------------------------------------" << endl;
        for (unsigned long i = addr1; i <= addr2; i+=4)
        {
            int val = returnDatas(i);
            cout << "0x" << hex << i << ": 0x" << val << endl;
        }
    }
    return 0;
}


