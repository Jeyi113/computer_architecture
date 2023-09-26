#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <exception>
#include <bitset>
#include "Directive.h"

using namespace std;

class Directive;
class Words;
class Instruction;
class Rtype;
class Itype;
class Jtype;
class Convert;

bool isIn(string str1, string r1);
bool isInt(string str0);
long long setToDec(string v);
string pureWord(string str2, string r2);
string dToh(int decimal);
string bToh(string bits);
string intoBit(long op, int rs, int rt, int rd, int sh, long f, long long imm, long long target, string type);
int registerNum(string res);

map<string, long long> lmap; // stores label and its address, global


vector<Directive> readFile(string file) // read the file and store the datas through instance.
{
    ifstream rd(file);
    string content; // sentences (lines) from the file
    vector<string> sen; // stores each sentence (line)
    vector<Directive> fileData; // stores specific datas
    int senlen; // # of lines
    int row = 5;

    //1. file open, store each sentence (line)
    if(rd.is_open())
    {
        while(rd.peek() != EOF)
        {
            // line by line
            getline(rd, content);
            sen.push_back(content);
        }
        senlen = sen.size();
        rd.close();
    }
    else {cout << "The file is not opened." << endl;}

    //2. Array for the data cell
    string dataCell[senlen][row];
    string directive;
    string label = " ";
    string previousLabel;
    long long wordAdrs = 0x10000000;
    long long textAdrs = 0x00400000;

    //3. store each word in arr
    Directive dt;
    Words ws;
    Instruction in;
    for (int i=0; i<senlen; i++)
    {
        stringstream ssm(sen[i]);
        for (int j=0; j<row; j++)
        { ssm >> dataCell[i][j]; }

        string op, v1, v2, v3; // for instruction seperates

        // A.starts with label:
        if ( isIn(dataCell[i][0],":"))
        {   
            previousLabel = label;
            label = pureWord(dataCell[i][0],":"); 
            // A-1. label: (only)
            if (dataCell[i][1].empty())
            {
                if(fileData[i-1].getDirect() == ".text") // .text
                { 
                    if (previousLabel != label){lmap.insert(pair<string, long long>(label,textAdrs));}
                    Directive d1(label,textAdrs, 1);
                    fileData.push_back(d1);
                }
                else
                {   
                    if (previousLabel != label){lmap.insert(pair<string, long long>(label,wordAdrs));}
                    Directive d2(label,wordAdrs, 1);
                    fileData.push_back(d2);
                }
            }
            // A-2) label: ~~
            else 
            {   // A-2.1) label: .directive 1 value
                if (isIn(dataCell[i][1],"."))
                {
                    if (previousLabel != label){lmap.insert(pair<string,long long>(label,wordAdrs));}
                    Words w1(label, dataCell[i][2], wordAdrs);
                    wordAdrs += 4;
                    fileData.push_back(w1);     
                }
                else // A-2.2) label: instructions
                {   
                    if (previousLabel != label){lmap.insert(pair<string, long long>(label,textAdrs));}
                    // 3 cases - # of value
                    op = dataCell[i][1];
                    v1 = dataCell[i][2];
                    v2 = dataCell[i][3];
                    v3 = dataCell[i][4];

                    if (isIn(v2,",")) // 3
                    {
                        vector<string> instruction{op,v1,v2,v3};
                        Instruction i1(label, instruction, textAdrs);
                        fileData.push_back(i1);
                    }
                    else if (isIn(v1,",")) // 2 (op : "la" must be converted the other instruction)
                    {
                        // "la" case
                        if (op == "la")
                        {
                            string upper = dToh(lmap[v3]); // load address
                            string lower = upper;

                            lower.erase(lower.begin(),lower.end()-4); lower = "0x" + lower;
                            upper.erase(upper.end()-4,upper.end()); upper = "0x" + upper;

                            if(lower == "0x0000")
                            {   /*lui*/ vector<string> instruction{"lui", v2, upper}; Instruction i5(label, instruction, textAdrs);
                                fileData.push_back(i5);
                            }
                            else
                            {
                                /*lui*/ vector<string> instruction{"lui", v2, upper}; Instruction i5(label, instruction, textAdrs);
                                fileData.push_back(i5);
                                textAdrs += 4;
                                /*ori*/ vector<string> instruction2{"ori",v2, v2, lower}; Instruction i6(label,instruction2, textAdrs);
                                fileData.push_back(i6);
                            }
                        }
                        else 
                        {
                            vector<string> instruction{op,v1,v2};
                            Instruction i2(label, instruction, textAdrs);
                            fileData.push_back(i2);
                        }
                    }
                    else // 1
                    {
                        vector<string> instruction{op,v1};
                        Instruction i3(label, instruction, textAdrs);
                        fileData.push_back(i3);
                        Instruction* temp = (Instruction*)(&fileData[i]);
                    }
                    textAdrs += 4;
                }
            }
        }
        // B. No label:
        else // arr[i][0] = 1) directives, 2)instructions, 3)null (exception)
        {   
            try
            {
                if (dataCell[i][0].empty()) throw i; // null line
                // B-1. starts with directives
                if ( isIn(dataCell[i][0],"."))
                {
                    directive = dataCell[i][0];
                    // B-1.1) directives (only)
                    if (dataCell[i][1].empty())
                    {   //.text only
                        if (directive == ".text")
                        {   
                            Directive d3(directive, textAdrs, 0);
                            fileData.push_back(d3);
                        }
                        else //.data, .word only
                        {
                            Directive d4(directive, wordAdrs, 0);
                            fileData.push_back(d4);
                        }
                    }
                    else // B-1-2) .word 1 variable
                    {   
                        Words w2(label, dataCell[i][1], wordAdrs);
                        wordAdrs += 4;
                        fileData.push_back(w2);
                    }
                }
                else // starts with Instructions
                {
                    op = dataCell[i][0];
                    v1 = dataCell[i][1];
                    v2 = dataCell[i][2];
                    v3 = dataCell[i][3];
                    if (isIn(v2,",")) // 3
                    {
                        vector<string> instruction{op,v1,v2,v3};
                        Instruction i4(label, instruction, textAdrs);
                        fileData.push_back(i4);
                    }
                    else if (isIn(v1,",")) // 2 (op = "la" must be converted to other instruction)
                    {
                        // "la" case
                        if (op == "la")
                        {
                            string upper = dToh(lmap[v2]); // load address
                            string lower = upper;

                            lower.erase(lower.begin(),lower.end()-4); lower = "0x" + lower;
                            upper.erase(upper.end()-4,upper.end()); upper = "0x" + upper;

                            if(lower == "0x0000")
                            {   /*lui*/ vector<string> instruction{"lui", v1, upper}; Instruction i5(label, instruction, textAdrs);
                                fileData.push_back(i5);
                            }
                            else
                            {
                                /*lui*/ vector<string> instruction{"lui", v1, upper}; Instruction i5(label, instruction, textAdrs);
                                fileData.push_back(i5);
                                textAdrs += 4;
                                /*ori*/ vector<string> instruction2{"ori",v1, v1, lower}; Instruction i6(label,instruction2, textAdrs);
                                fileData.push_back(i6);
                            }
                        }
                        else
                        {
                            vector<string> instruction{op,v1,v2};
                            Instruction i5(label, instruction, textAdrs);
                            fileData.push_back(i5);
                        }
                    }
                    else // 1
                    {
                        vector<string> instruction{op,v1};
                        Instruction i6(label, instruction, textAdrs);
                        fileData.push_back(i6);
                    }
                    textAdrs += 4;
                }
            }
            catch(int i){Directive d5(" ", -1, 1); fileData.push_back(d5);}
        }
    }
    return fileData;
}

void writeFile(vector<Directive> data, string file)
{
    // 1. Prepare for the contents 
    int textSize = 0;
    int dataSize = 0;
    vector<int> value;
    vector<string> bits;

    // 1) Section size calculate, 2) convert value (hex), 3) convert instruction (binary->hex)
    for (int i = 0; i < data.size(); i++)
    {
        Directive* d;
        d = &data[i];
        if (d->getNum() == 0)
        {
            if (d->getDirect() == ".text") // class Instruction
            {
                // 1) text section size 
                textSize += 4;

                // 2) Instruction convert to hex
                vector<string> iline = d->getInst(); // instruction line
                string type = d->getType();
                string opc = iline[0]; // opcode
                string finalBit; // instruction bit line.
                long long imm = 0; // imm (in Itype - offset)
                long long jt = 0; // jump target (in Jtype - target address / 4)
                long o = 0;
                long f = 0; // function
                int rs = 0; // rs, jump target(in jr)
                int rt = 0; // rt
                int rd = 0; // rd
                int sh = 0; // shift amount
                Instruction in;
                map<string,vector<string>> imap = in.getMap(); // instruction map

                if (type == "R") // Rtype
                {   
                    f = stoul(imap[opc][1], nullptr, 16);
                    o = stoul(imap[opc][0], nullptr, 16);

                    if(d->getInst().size() <= 2) // 1) jr
                    {rs = registerNum(iline[1]); rt = 0; rd = 0;}
                    else
                    {   
                        if(f == 0 || f == 2) // 2) shift
                        {/*shift*/rt = registerNum(iline[2]); rd = registerNum(iline[1]); sh = setToDec(iline[3]);}
                        else // 3) normal
                        {rs = registerNum(iline[2]); rt = registerNum(iline[3]); rd = registerNum(iline[1]);}
                    }
                }
                else if (type == "I") // Itype
                {
                    o = stoul(imap[opc][0], nullptr, 16);

                    if (opc == "lui"){rt = registerNum(iline[1]); imm = setToDec(iline[2]); }
                    else
                    {
                        if (opc == "bne" || opc == "beq") // 1) bne, beq (has a label)
                        {/*beq, bne*/ rs = registerNum(iline[1]); rt = registerNum(iline[2]); imm = (lmap[iline[3]] - (d->getAdrs()))/4; } // imm = offset
                        else if (isIn(iline[2],"($")) // 2) offset : op rt offset($rs)
                        {/*offset*/ 
                            string reg = iline[2];
                            string offset = reg;
                            offset.erase(offset.find("("),offset.find(")")); 
                            int gap = offset.size()+1;
                            reg.erase(reg.begin(), reg.begin()+gap);
                            reg.erase(reg.end()-1);

                            rs = registerNum(reg); rt = registerNum(iline[1]); imm = setToDec(offset);
                        }
                        else // constant imm
                        {/*constant*/ rs = registerNum(iline[2]); rt = registerNum(iline[1]); imm = setToDec(iline[3]);}
                    }
                }
                else if (type == "J") // Jtype
                {
                    o = stoul(imap[opc][0], nullptr, 16);
                    long long target = lmap[iline[1]];
                    jt = target / 4;
                }
                finalBit = intoBit(o, rs, rt, rd, sh, f, imm, jt, type);
                bits.push_back(bToh(finalBit));
                //cout << "The bitline is : " << finalBit << endl;
                //cout << "The hexa ver is : " << bToh(finalBit) << endl;
            }
            else // class Words
            {
                dataSize += 4;
                // convert value
                string val = d->getVal();
                if (!isInt(val)) {value.push_back(stoul(val,nullptr,16));}
                else {int vval = stoi(val); value.push_back(vval);}
            }
        }
    }
    // 

    // 2. Write file
    ofstream wf(file);
    if(wf.is_open())
    {
        // Section size
        wf << "0x" << hex << textSize << endl;
        wf << "0x" << hex << dataSize << endl;
        // Instructions
        for (int i = 0; i < bits.size(); i++){ wf << "0x" << bits[i] << endl;}
        // Data value
        for (int i = 0; i < value.size(); i++){ wf << "0x" << hex << value[i] << endl;}
        wf.close();
    }
}

bool isIn(string str1, string r1) // to see that r1 is in str1
{
    if(str1.find(r1) != string::npos)
    {
        return true;
    }
    else {return false;}
}

bool isInt(string str0) // to see that str0 is int(1) of hex(0)
{
    str0.erase(str0.begin()+2,str0.end());
    if(str0 == "0x"){return false;}
    else {return true;}
}

string pureWord(string str2, string r2) // remove r2 from str2
{
    int index = str2.find(r2);
    str2.erase(index);
    return str2;
}

long long setToDec(string v)
{
    long long value = stoull(v);
    if (!isIn(v,"$")&&!isInt(v))
    {value = stoull(v,nullptr,16);}

    return value;
}

string dToh(int decimal)
{
    string hexa_str;
    stringstream ss;
    ss << hex << decimal;
    hexa_str = ss.str();

    return hexa_str;
}

string bToh(string bits)
{
    string hexa_str;
    stringstream ss;
    ss << hex << stoul(bits,nullptr,2);
    hexa_str = ss.str();

    return hexa_str;
}

string intoBit(long op, int rs, int rt, int rd, int sh, long f, long long imm, long long target, string type)
{
    string bitline;
    bitset<6> b1(op); bitset<5> b2(rs); bitset<5> b3(rt); bitset<5> b4(rd); bitset<5> b5(sh); bitset<6> b6(f); bitset<16> b7(imm); bitset<26> b8(target);
    
    if (type == "R"){bitline = b1.to_string() + b2.to_string() + b3.to_string() + b4.to_string() + b5.to_string() + b6.to_string();}
    else if (type == "I"){bitline = b1.to_string() + b2.to_string() + b3.to_string() + b7.to_string();}
    else {bitline = b1.to_string() + b8.to_string();}

    return bitline;
}

int registerNum(string res) // remove "$" from the register name)
{
    int rNum;
    auto it = res.begin();
    res.erase(it);
    rNum = stoi(res,nullptr,10); // register Number is dec.

    return rNum;
}


int main(int argc, char* argv[])
{
    // 1. get the name of the file
    /*string filename;
    cout << "Enter the name of the file to execute. : ";
    cin >> filename;*/
    string filename = "";
    if (argc == 2){filename = argv[1];}
    else {cout << "Enter the file name : "; return 1;}

    // 2. read file and get datas
    vector<Directive> datas = readFile(filename);

    // 3. Convert into hex file
    auto it = filename.end();
    filename.erase(it-1);
    filename.append("o");
    /*cout << "the new file name is : " << filename << endl;*/
    writeFile(datas, filename);
    
    return 0;
}
