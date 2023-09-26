#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <bitset>
#include <unistd.h>
#include "StageRegister.h"

using namespace std;

unsigned int PC = 0x400000;
unsigned int PC_IF = 0x400000, PC_ID = 0, PC_EX = 0, PC_MEM = 0, PC_WB = 0;
vector<pair<unsigned int,bitset<32>>> textRegister, dataRegister;
map<string,long> R = {{"R0",0},{"R1", 0},{"R2",0},{"R3",0},{"R4",0},{"R5",0},{"R6",0},{"R7",0},{"R8",0},{"R9",0},{"R10",0},{"R11",0},{"R12",0},{"R13",0},{"R14",0},{"R15",0},{"R16",0},{"R17",0},{"R18",0},{"R19",0},{"R20",0},{"R21",0},{"R22",0},{"R23",0},{"R24",0},{"R25",0},{"R26",0},{"R27",0},{"R28",0},{"R29",0},{"R30",0},{"R31",0}};
IF_ID fd;
ID_EX dx;
EX_MEM em;
MEM_WB mw;
bitset<2> ForwardA = 0b00, ForwardB = 0b00;
bitset<1> ForwardC = 0b0, ForwardD = 0b0;
bool PCSrc, Jump = 0;
bool PCWrite = 1, fdWrite = 1;

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

void EXForwardUnit()
{
    /* EX Forward Unit*/
    if (em.getRegWrite() && (em.getRd() != 0) && (em.getRd() == dx.getRs())) // R
    {ForwardA = 0b10;} // (R)em.rd == (R or I)dx.rs 
    if (em.getRegWrite() && (em.getRd() != 0) && (dx.getOp() == 0) && (em.getRd() == dx.getRt())) // R
    {ForwardB = 0b10;} // (R)em.rd == (R)dx.rt
    if (em.getRegWrite() && (em.getOp() !=0) && (em.getRt() != 0) && (em.getRt() == dx.getRs())) // I
    {ForwardA = 0b10;} // (I)em.rt == (R or I)dx.rs
    if (em.getRegWrite() && (em.getOp() !=0) && (em.getRt() != 0) && (dx.getOp() == 0) && (em.getRt() == dx.getRt()))
    {ForwardB = 0b10;} // (I)em.rt == (R)dx.rt
}

void MEMForwardUnit()
{
    /*MEM Forward Unit*/
    if (mw.getRegWrite() && (mw.getRd() != 0) && (em.getDst() != dx.getRs()) && (mw.getRd() == dx.getRs()))
    {ForwardA = 0b01;} // (R)mw.rd == (R or I)dx.rs (em.dst != dx.rs)
    if (mw.getRegWrite() && (mw.getRd() != 0) && (dx.getOp() == 0) && (em.getDst() != dx.getRt()) && (mw.getRd() == dx.getRt()))
    {ForwardB = 0b01;} // (R)mw.rd == (R)dx.rt (em.dst != dx.rt)
    if (mw.getRegWrite() && (mw.getOp() != 0) && (mw.getRt() != 0) && (em.getDst() != dx.getRs()) && (mw.getRt() == dx.getRs()))
    {ForwardA = 0b01;} // (I)mw.rt == (R or I)dx.rs (em.dst != dx.rs)
    if (mw.getRegWrite() && (mw.getOp() != 0) && (mw.getRt() != 0) && (dx.getOp() == 0) && (em.getDst() != dx.getRt()) && (mw.getRt() == dx.getRt()))
    {ForwardB = 0b01;} // (I)mw.rt == (R)dx.rt (em.dst != dx.rt)
}
void IDForwardUnit()
{
    /*ID Forward Unit*/
    if (fd.getBrch() && (em.getDst() != 0) && (em.getDst() == fd.getRs())){ForwardC = 0b1;}
    if (fd.getBrch() && (em.getDst() != 0) && (em.getDst() == fd.getRt())){ForwardD = 0b1;}
}

long FindValue(int adrs)
{
    for (int i = 0; i < dataRegister.size(); i++)
    {if (dataRegister[i].first == adrs){return stol(dataRegister[i].second.to_string(),nullptr,2);}}
    return -1;
}

int FindValueB(int adrs)
{
    for (int i = 0; i < dataRegister.size(); i++)
    {
        bitset<8> b1 = (dataRegister[i].second.to_ulong() >> 24) & 0b11111111;
        bitset<8> b2 = (dataRegister[i].second.to_ulong() >> 16) & 0b11111111;
        bitset<8> b3 = (dataRegister[i].second.to_ulong() >> 8) & 0b11111111;
        bitset<8> b4 = (dataRegister[i].second.to_ulong() >> 0) & 0b11111111;
        if (dataRegister[i].first == adrs){return stoi(b1.to_string(),nullptr,2);} 
        else if (dataRegister[i].first + 1 == adrs){return stoi(b2.to_string(),nullptr,2);}
        else if (dataRegister[i].first + 2 == adrs){return stoi(b3.to_string(),nullptr,2);}
        else if (dataRegister[i].first + 3 == adrs){return stoi(b4.to_string(),nullptr,2);}
    }
    return -1;
}

void SetWords(int adrs, bitset<32> bi)
{
    int cover = 0;
    for (int i = 0; i < dataRegister.size(); i++)
    {
        if (dataRegister[i].first == adrs){cover = 1; dataRegister[i].second = bi; cout << "Changed data : " << dataRegister[i].second << endl;}
    }
    if (cover != 1){dataRegister.push_back({adrs,bi});} // add new memory
}

void SetWordsB(int adrs, bitset<8> bi)
{   
    int cover = 0;
    for (int i = 0; i < dataRegister.size(); i++)
    {   
        string val;
        bitset<8> b1 = (dataRegister[i].second.to_ulong() >> 24) & 0b11111111;
        bitset<8> b2 = (dataRegister[i].second.to_ulong() >> 16) & 0b11111111;
        bitset<8> b3 = (dataRegister[i].second.to_ulong() >> 8) & 0b11111111;
        bitset<8> b4 = (dataRegister[i].second.to_ulong() >> 0) & 0b11111111;
        if (dataRegister[i].first == adrs){cover = 1; bitset<32> combine = (bi.to_ulong() << 24)|(b2.to_ulong() << 16)|(b3.to_ulong() << 8)|(b4.to_ulong()); dataRegister[i].second = combine;}
        else if (dataRegister[i].first + 1 == adrs){cover = 1; bitset<32> combine = (b1.to_ulong() << 24)|(bi.to_ulong() << 16)|(b3.to_ulong() << 8)|(b4.to_ulong()); dataRegister[i].second = combine;}
        else if (dataRegister[i].first + 2 == adrs){cover = 1; bitset<32> combine = (b1.to_ulong() << 24)|(b2.to_ulong() << 16)|(bi.to_ulong() << 8)|(b4.to_ulong()); dataRegister[i].second = combine;}
        else if (dataRegister[i].first + 3 == adrs){cover = 1; bitset<32> combine = (b1.to_ulong() << 24)|(b2.to_ulong() << 16)|(b3.to_ulong() << 8)|(bi.to_ulong()); dataRegister[i].second = combine;}
    }
    if (cover != 1){bitset<32> b; b |= (bi.to_ulong() << 24); dataRegister.push_back({adrs,b});}
}

void IF(unsigned int pc) // Read 1 instruction from memory
{
    if (fdWrite == 1 || pc != 0)
    {
        fd.setForward(0);
        if (pc != 0)
        {
            fd.setNoop(0); 

            // Control Hazard Unit (Flush)
            if (dx.getJump() == 1){/*Flush*/fd.setNoop(1); Jump = 1;}

            if (fdWrite == 1 && Jump != 1)
            {
                bitset<32> instruction;
                for (int i = 0; i < textRegister.size(); i++){if (pc == textRegister[i].first){instruction = textRegister[i].second;}}

                // Update IF/ID stage register (Instr, NPC, op, rs, rt, brch) -> op, rs, rt, brch for check forwarding
                bitset<6> op((instruction.to_ulong() >> 26) & 0b111111);
                bitset<5> rs((instruction.to_ulong() >> 21) & 0b11111);
                bitset<5> rt((instruction.to_ulong() >> 16) & 0b11111);
                fd.setInst(instruction); fd.setNPC(PC); fd.setOp(op); fd.setRs(rs); fd.setRt(rt);
            
                if (op == 0b000100 || op == 0b000101){fd.setBrch(1);}
                else {fd.setBrch(0);}
            }
        }
        else {fd.setNoop(1);} // noop
    }
}
void ID(unsigned int pc) // Decode instruction and read register data needed
{
    if (pc != 0)
    {
        dx.setNoop(0);

        // Data Hazard Unit (bubble)
        if ((dx.getMemRead() == 1) && ((dx.getRt() == fd.getRs()) || (dx.getRt() == fd.getRt())))
        {PCWrite = 0, fdWrite = 0;}
        else if ((fd.getBrch() == 1) && ((dx.getRegDst() == 1 && dx.getRd() !=0 && (dx.getRd() == fd.getRs() || dx.getRd() == fd.getRt())) || dx.getRegDst() == 0 && dx.getRt() != 0 && (dx.getRt() == fd.getRs() || dx.getRt() == fd.getRt())))
        {PCWrite = 0, fdWrite = 0;}

        // Get datas from IF/ID register
        bitset<32> instruction;
        unsigned int NPC;
        for (int i = 0; i < textRegister.size(); i++){if (textRegister[i].first == pc){instruction = textRegister[i].second;}}
        if (fd.getInst().to_ulong() != 0){NPC = fd.getNPC();}
        else {NPC = pc + 4;}

        // Decode - 1. Read Value from the register file (rs, rt, IMM, rd)
        bitset<6> opcode((instruction.to_ulong() >> 26) & 0b111111);
        int op = opcode.to_ulong();
        if (op == 0) // R
        {
            bitset<5> rs((instruction.to_ulong() >> 21) & 0b11111);
            bitset<5> rt((instruction.to_ulong() >> 16) & 0b11111);
            bitset<5> rd((instruction.to_ulong() >> 11) & 0b11111);
            bitset<5> sh((instruction.to_ulong() >> 6) & 0b11111);
            bitset<6> fn(instruction.to_ulong() & 0b111111);
            long d1 = R["R"+to_string(rs.to_ulong())];
            long d2 = R["R"+to_string(rt.to_ulong())];
            dx.setOp(opcode); dx.setRs(rs); dx.setRt(rt); dx.setRd(rd); dx.setSh(sh); dx.setFn(fn);
            dx.setData1(d1); dx.setData2(d2);

            // sll, srl
            if (fn == 0b000000){bitset<32> shift(d2 << sh.to_ulong()); dx.setShift(shift);} // sll
            else if (fn == 0b000010){bitset<32> shift(d2 >> sh.to_ulong()); dx.setShift(shift);} // srl
            else if (fn == 0b001000){PC = d1;} // jr
        }
        else
        {
            if ( op == 2 || op == 3) // J
            {
                bitset<26> jt(instruction.to_ulong() & 0b11111111111111111111111111);
                if (op == 2) // J
                {PC = (jt << 2).to_ulong();}
                else  // Jal
                {PC = ((PC & 0xf0000000)|(jt << 2).to_ulong()); dx.setShift(NPC);} 
                dx.setOp(opcode); dx.setRs(0); dx.setRt(0); dx.setRd(0);
            }
            else // I
            {    
                bitset<5> rs((instruction.to_ulong() >> 21) & 0b11111);
                bitset<5> rt((instruction.to_ulong() >> 16) & 0b11111);
                bitset<16> imm(instruction.to_ulong() & 0b1111111111111111);
                
                bool msb = imm[15];
                bitset<32> e = msb ? (imm.to_ulong() | 0xFFFF0000) : imm.to_ulong(); // sign extend (16->32)
                
                long d1 = R["R"+to_string(rs.to_ulong())];
                long d2 = stol(e.to_string(),nullptr,2);               
                dx.setOp(opcode); dx.setRs(rs); dx.setRt(rt); dx.setRd(0); dx.setImm(e);
                dx.setData1(d1); dx.setData2(d2); 
                
                if (op == 0xf){bitset<32> luiimm(imm.to_ulong() << 16); dx.setShift(luiimm);} // lui
                if (op == 0x4 || op == 0x5) // bne, beq
                {
                    bitset<5> temp = rs; rs = rt; rt = temp;
                    long d2 = R["R"+to_string(rt.to_ulong())]; dx.setData2(d2);

                    unsigned int bt = NPC + (e << 2).to_ulong();
                    dx.setBt(bt); dx.setRs(rs); dx.setRt(rt);
                }
                
                // Branch data forwarding - Doesn't need it?
                if (dx.getBrch() == 1)
                {
                    if (fd.getForward() == 1){dx.setData1(fd.getData1());}
                    if (fd.getForward() == 2){dx.setData2(fd.getData2());}
                }
            }
        }

        // Decode - 2. Read Instruction opcode, function fields -> send to Control Unit
        // Control signal : EX(RegDst, ALUOp1, ALUOp2, ALUSrc) + M(Brch, MemRead, MemWrite) + WB(RegWrite, MemtoReg) (* -1 : not used)
        if (op == 0x0)// R type
        {
            if (dx.getFn() == 0b000000 || dx.getFn() == 0b000010) // sll, srl
            {dx.setJump(0); dx.setRegDst(1); dx.setALUOp1(-1); dx.setALUOp2(-1); dx.setALUSrc(-1); dx.setBrch(0); dx.setMemRead(0); dx.setMemWrite(0); dx.setRegWrite(1); dx.setMemtoReg(2);}
            else if (dx.getFn() == 0b001000) // jr
            {dx.setJump(1); dx.setRegDst(-1); dx.setALUOp1(-1); dx.setALUOp2(-1); dx.setALUSrc(-1); dx.setBrch(0); dx.setMemRead(0); dx.setMemWrite(0); dx.setRegWrite(-1); dx.setMemtoReg(-1);}
            else // else
            {dx.setJump(0); dx.setRegDst(1); dx.setALUOp1(1); dx.setALUOp2(0); dx.setALUSrc(0); dx.setBrch(0); dx.setMemRead(0); dx.setMemWrite(0); dx.setRegWrite(1); dx.setMemtoReg(0);}
        }
        else if (op == 0x9 || op == 0xc || op == 0xd || op == 0xb){dx.setJump(0); dx.setRegDst(0); dx.setALUOp1(-1); dx.setALUOp2(-1); dx.setALUSrc(1); dx.setBrch(0); dx.setMemRead(0); dx.setMemWrite(0); dx.setRegWrite(1); dx.setMemtoReg(0);} // I type (addiu, andi, ori, sltiu)
        else if (op == 0x4 || op == 0x5){dx.setJump(0); dx.setRegDst(-1); dx.setALUOp1(0); dx.setALUOp2(1); dx.setALUSrc(0); dx.setBrch(1); dx.setMemRead(0); dx.setMemWrite(0); dx.setRegWrite(0); dx.setMemtoReg(-1);} // beq, bne
        else if (op == 0x20 || op == 0x23){dx.setJump(0); dx.setRegDst(0); dx.setALUOp1(0); dx.setALUOp2(0); dx.setALUSrc(1); dx.setBrch(0); dx.setMemRead(1); dx.setMemWrite(0); dx.setRegWrite(1); dx.setMemtoReg(1);} // lb, lw
        else if (op == 0x28 || op == 0x2b){dx.setJump(0); dx.setRegDst(-1); dx.setALUOp1(0); dx.setALUOp2(0); dx.setALUSrc(1); dx.setBrch(0); dx.setMemRead(0); dx.setMemWrite(1); dx.setRegWrite(0); dx.setMemtoReg(-1);} // sb, sw
        else if (op == 0x2){dx.setJump(1); dx.setRegDst(-1); dx.setALUOp1(-1); dx.setALUOp2(-1); dx.setALUSrc(-1); dx.setBrch(0); dx.setMemRead(0); dx.setMemWrite(0); dx.setRegWrite(0); dx.setMemtoReg(-1);} // J
        else if (op == 0x3){dx.setJump(1); dx.setRegDst(2); dx.setALUOp1(-1); dx.setALUOp2(-1); dx.setALUSrc(-1); dx.setBrch(0); dx.setMemRead(0); dx.setMemWrite(0); dx.setRegWrite(1); dx.setMemtoReg(3);} // Jal
        else {dx.setJump(0); dx.setRegDst(0); dx.setALUOp1(-1); dx.setALUOp2(-1); dx.setALUSrc(-1); dx.setBrch(0); dx.setMemRead(0); dx.setMemWrite(0); dx.setRegWrite(1); dx.setMemtoReg(2);} // I type (lui)

        // Update ID/EX stage register (NPC)
        dx.setNPC(NPC);
        if (PC_IF != 0){fd.setNoop(0);}

        // Flush IF stage
        if (dx.getJump() == 1){PC_IF = 0;}
    }
    else {dx.setNoop(1);} // noop
}
void EX(unsigned int pc) // Execute ALU
{
    if (pc != 0)
    {
        em.setNoop(0);
        // Control signal used : RegDst, ALUOp1, ALUOp2, ALUSrc
        unsigned int NPC = dx.getNPC();
        unsigned int dst = dx.getRegDst(); // RegDst = 2 : R31, 1 : rd, 0 : rt
        unsigned int op = dx.getOp();
        unsigned int destination; // destination register number
        if (dst == 2){destination = 31;} // R31 (jr)
        else if (dst){destination = dx.getRd();} // rd 
        else {destination = dx.getRt();} // rt

        int ALUOp1 = dx.getALUOp1();
        int ALUOp2 = dx.getALUOp2();
        int ALUSrc = dx.getALUSrc();

        // ALU Calculation
        /*1. ALU Control*/
        map <int,bitset<4>> AC = {{33,0b0010},{35,0b0110},{36,0b0000},{37,0b0001},{43,0b0111},{39,0b1100}};
        string ALUOP = to_string(ALUOp1) + to_string(ALUOp2);
        bitset<4> ctrl;
        if (ALUOP == "00"){ctrl = 0b0010;} // add (lw, lb, sw, sb)
        else if (ALUOP == "01"){ctrl = 0b0110;} // subtract (beq)
        else if (ALUOP == "10"){ctrl = AC[dx.getFn()];} // R type
        else // No ALUOP - read opcode and set ctrl
        {
            if (op == 9){ctrl = 0b0010;} // addiu
            else if (op == 12){ctrl = 0b0000;} // andi
            else if (op == 13){ctrl = 0b0001;} // ori
            else if (op == 11){ctrl = 0b0111;} // sltiu
            else {ctrl = 0b1111;} // no ctrl
        }

        /*2. ALU Calculation*/
        long oprd1 = dx.getData1(), oprd2 = dx.getData2();
        long alu_out;

        if (ctrl == 0b0010){alu_out = unsigned (oprd1 + oprd2);} // add
        else if (ctrl == 0b0110){alu_out = unsigned (oprd1 - oprd2);} // subtract
        else if (ctrl == 0b0000){alu_out = oprd1 & oprd2;} // AND
        else if (ctrl == 0b0001){alu_out = oprd1 | oprd2;} // OR
        else if (ctrl == 0b0111){if(oprd1 < oprd2){alu_out = 1;} else{alu_out = 0;}} // set less than
        else if (ctrl == 0b1100){alu_out = unsigned (~(oprd1 | oprd2));} // NOR
        else if (ctrl == 0b1111){} // nothing 
        else {cout << "ALUControl ERROR" << endl;}

        /*3. Shift Hardware - forwarding*/
        unsigned int fn = dx.getFn();
        long shift = dx.getShift();
        if (op == 0 && fn == 0b000000){shift = (oprd2 << dx.getSh());} // sll
        else if (op == 0 && fn == 0b000010){shift = (oprd2 >> dx.getSh());} // srl


        // Update EX/MEM stage register (ALU_OUT, BR_TARGET, NPC, op, rs, rt, rd, shift(extended), imm, Control signal)
        em.setOp(op); em.setDst(destination); em.setNPC(NPC); em.setALU(alu_out); em.setBt(dx.getBt()); em.setRs(dx.getRs()); em.setRt(dx.getRt()); em.setRd(dx.getRd()); em.setShift(shift); em.setImm(dx.getImm());
        // Control signal : M(Brch, MemRead, MemWrite) + WB(RegWrite, MemtoReg)
        em.setBrch(dx.getBrch()); em.setMemRead(dx.getMemRead()); em.setMemWrite(dx.getMemWrite()); em.setRegWrite(dx.getRegWrite()); em.setMemtoReg(dx.getMemtoReg());
    }
    else {em.setNoop(1);} // noop
}
void MEM(unsigned int pc) // Memory access (load, store)
{
    if (pc != 0)
    {
        mw.setNoop(0);
        // Control signal used : Brch, MemRead, MemWrite
        unsigned int NPC = em.getNPC();
        unsigned int brch = em.getBrch();
        unsigned int memRead = em.getMemRead(); mw.setMemRead(memRead);
        unsigned int memWrite = em.getMemWrite(); mw.setMemWrite(memWrite);
        unsigned int op = em.getOp();
        unsigned int destination = em.getDst();
        long alu = em.getALU();
        long mo; // mem_out value
        long bTarget;

        if (brch) // branch
        {
            bitset<32> imm = em.getImm();
            if (op == 0x4){if (alu == 0){bTarget = NPC + (imm << 2).to_ulong(); mw.setBt(bTarget);}} // beq
            else if (op == 0x5){if (alu != 0){bTarget = NPC + (imm << 2).to_ulong(); mw.setBt(bTarget);}} // bne
        }
        if (memRead) // read memory (load)
        {
            if (op == 0x23){mo = FindValue(alu);} // lw
            else if (op == 0x20){mo = FindValueB(alu);} // lb
        } 
        if (memWrite)
        {
            if (op == 0x2b){bitset<32> b(R["R" + to_string(em.getRt())]); SetWords(alu,b);} // sw
            else if (op == 0x28){bitset<8> b(R["R"+to_string(em.getRt())] & 255); SetWordsB(alu,b);} // sb
        } // write to memory (store)

        // Update MEM/WB stage register (ALU_OUT, MEM_OUT, DstReg, NPC, op, rs, rt, rd, shift, brch, Control signal)
        mw.setALU(alu); mw.setMemOut(mo); mw.setDst(destination); mw.setNPC(em.getNPC()); mw.setOp(op); mw.setRs(em.getRs()); mw.setRt(em.getRt()); mw.setRd(em.getRd()); mw.setShift(em.getShift()); mw.setBrch(brch);
        // Control signal : WB(RegWrite, MemtoReg)
        mw.setRegWrite(em.getRegWrite()); mw.setMemtoReg(em.getMemtoReg());

    }
    else {mw.setNoop(1);} // noop
}
void WB(unsigned int pc) // Write back the ALU result to register
{
    if (pc != 0)
    {
        unsigned int destination = mw.getDst(); // destination reg num
        // Control signal used : RegWrite, MemtoReg
        if (mw.getRegWrite())
        {
            if (mw.getMemtoReg() == 0){R["R"+to_string(destination)] = mw.getALU();} // alu result
            else if (mw.getMemtoReg() == 1){R["R"+to_string(destination)] = mw.getMem();} // memory data
            else if (mw.getMemtoReg() == 2 || mw.getMemtoReg() == 3){R["R"+to_string(destination)] = mw.getShift();} // sll srl lui jal
        } 
    }
}  

string sPC(unsigned int pc)
{
    string spc;
    if(pc != 0 && pc < 0x400000 + 4*textRegister.size())
    {
        stringstream hex_pc;
        hex_pc << hex << pc;
        spc = hex_pc.str();
        spc = "0x" + spc;
    }
    else {spc = "";}
    return spc;
}

long returnDatas(unsigned long adrs)
{
    for (int i = 0; i < dataRegister.size(); i++){if (dataRegister[i].first == adrs){return dataRegister[i].second.to_ulong();}}
    for (int i = 0; i < textRegister.size(); i++){if (textRegister[i].first == adrs){return textRegister[i].second.to_ulong();}}
    return 0; // No matching
}

int main(int argc, char* argv[])
{
    // 0. Check the commend input
    string filename;
    string adrs;
    long addr1, addr2; // memory addr range
    int iNum = 1; // instruction number to execute
    int atp = 0, antp = 0, m_op = 0, d_op = 0, p_op = 0, n_op = 0;
    int opt;

    if (strcmp(argv[1], "-atp") == 0){atp = 1;}
    else if (strcmp(argv[1], "-antp") == 0){antp = 1;}
    else {cout << "The prediction mode must be given" << endl; return -1;}
    optind = 2;

    while ((opt = getopt(argc, argv, "m:dpn:")) != -1)
    {
        switch(opt){
            case 'a':
                if (optarg == nullptr){cout << "It is not valid flag." << endl;}
                else if (strcmp(optarg,"tp") == 0){cout << "atp" << endl; atp = 1;}
                else if (strcmp(optarg, "ntp") == 0){cout << "antp" << endl; antp = 1;}
                break;
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
            case 'p':
                p_op = 1;
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

    // 1. File read
    vector<bitset<32>> rawfile = FileIn(filename);
    // 2. Analayze Contents
    /*save to data register, instruction register*/
    unsigned int textAdrs = 0x400000, dataAdrs = 0x10000000;
    int textSize = rawfile[0].to_ulong()/4, dataSize = rawfile[1].to_ulong()/4;
    pair<unsigned int, bitset<32>> p;
    
    for (int i = 2; i < 2 + textSize; i ++){if(rawfile[i].to_ulong() != 0){p = {textAdrs,rawfile[i]}; textRegister.push_back(p); textAdrs += 4;}}
    for (int i = 2 + textSize; i < rawfile.size(); i++){p = {dataAdrs,rawfile[i]}; dataRegister.push_back(p); dataAdrs += 4;}
    textSize = textRegister.size(); // resize the textSize (since 0x0 instructions are included.)

    // 3. Pipeline (IF - ID - EX - MEM - WB)
    int cycle = 1;
    unsigned int range = 0x400000 + 4*textSize;
    while ((PC_WB < range) && iNum > 0)
    {
        string pcIF = sPC(PC_IF);
        if(pcIF != "" && PCWrite){PC += 4;} // update PC
        fdWrite = 1; Jump = 0;
        if (pcIF != "" || fdWrite){PCWrite = 1;} // reset fdWrite
        
        // Pipeline    

        IDForwardUnit();
        EXForwardUnit();
        MEMForwardUnit();

        if (ForwardA == 0b10) // em.alu -> dx.data1
        {
            if (em.getMemtoReg() == 2){dx.setData1(em.getShift());}
            else {dx.setData1(em.getALU());}
        }
        else if (ForwardA == 0b01) // mw.alu -> dx.data1
        {
            if (mw.getMemtoReg() == 2){dx.setData1(mw.getShift());} // sll, srl, lui, jal
            else 
            {
                if (mw.getMemRead() == 1 || mw.getMemWrite() == 1){dx.setData1(mw.getMem());}
                else {dx.setData1(mw.getALU());}
            }
        }
        if (ForwardB == 0b10) // em.alu -> dx.data2
        {
            if (em.getMemtoReg() == 2){dx.setData2(em.getShift());}
            else {dx.setData2(em.getALU());}
        }
        else if (ForwardB == 0b01) // mw.alu -> dx.data2
        {
            if (mw.getMemtoReg() == 2){dx.setData2(em.getShift());}
            else
            {
                if (mw.getMemRead() == 1 || mw.getMemWrite() == 1){dx.setData2(mw.getMem());}
                else {dx.setData2(mw.getALU());}
            }
        }
        if (ForwardC == 0b1) // em.alu -> fd.data1
        {  
            if (em.getMemtoReg() == 2){fd.setData1(em.getShift()); fd.setForward(1);}
            else {fd.setData1(em.getALU()); fd.setForward(1);}
        }
        if (ForwardD == 0b1) // em.alu -> fd.data2
        {   
            if (em.getMemtoReg() == 2){fd.setData2(em.getShift()); fd.setForward(2);}
            else {fd.setData2(em.getALU()); fd.setForward(2);}
        }
        ForwardA = 0b00; ForwardB = 0b00; ForwardC = 0b0; ForwardD = 0b0;

        WB(PC_WB); 
        MEM(PC_MEM);
        EX(PC_EX); 
        ID(PC_ID); 
        IF(PC_IF); 

        pcIF = sPC(PC_IF); string pcID = sPC(PC_ID), pcEX = sPC(PC_EX), pcMEM = sPC(PC_MEM), pcWB = sPC(PC_WB);
        
        if (pcIF == "" && pcID == "" && pcEX == "" && pcMEM == "" && pcWB == ""){}
        else
        {
            if (p_op) // -p (print pipeline PC state in every cycle)
            {
                cout << "===== Cycle " << dec << cycle << " =====" << endl;
                cout << "Current pipeline PC state:" << endl;
                cout << "{" << pcIF << "|" << pcID << "|" << pcEX << "|" << pcMEM << "|" << pcWB << "}\n" << endl;
            }
            if (d_op) // -d (print register values in every cycle)
            {
                cout << "Current register values:\nPC: 0x" << hex << PC << "\nRegisters:" << endl;
                for(int i = 0; i < R.size(); i++)
                {
                    string reg = "R"+to_string(i); cout << reg << ": 0x";
                    int convert = R[reg];
                    cout << hex << convert << endl;
                }
                cout << "\n"; 

                // with -m
                if (m_op)// -m
                {
                    cout << "Memory content [0x" << hex << addr1 << "..0x" << addr2 <<"]:" << endl;
                    cout << "------------------------------------" << endl;
                    for (unsigned long i = addr1; i <= addr2; i+=4)
                    {
                        int val = returnDatas(i);
                        cout << "0x" << hex << i << ": 0x" << val << "\n" << endl;
                    }
                }
            }
            if (n_op){if(pcWB != ""){iNum --;}} // -n
            cycle += 1;
        }
        
        // Update
        PC_WB = PC_MEM; PC_MEM = PC_EX; PC_EX = PC_ID;
        if (fdWrite){PC_ID = PC_IF; PC_IF = PC;}
        else if (fdWrite == 0){PC_EX = 0;} // Bubble

        // Branch Prediction
        /*Predic not taken : antp*/
        if (antp) 
        {
            if (em.getBrch() == 1 && ((em.getOp() == 0x4 && em.getALU() == 0) || (em.getOp() == 0x5 && em.getALU() != 0)))
            {PC_EX = 0; PC_ID = 0; PC_IF = 0; PC = em.getBt();} // Branch taken (Predict not taken)
        }
        /*Predic taken : atp*/
        else if (atp)
        {
            if (fd.getBrch() == 1){/*noop*/fdWrite = 0; PCWrite = 0; PC_IF = 0;}
            if (dx.getBrch() == 1){PC = dx.getBt();}
            if (em.getBrch() == 1 && ((em.getOp() == 0x4 && em.getALU() != 0) || (em.getOp() == 0x5 && em.getALU() == 0))) // Branch not taken (Predict not taken) - flush 3
            {PC_IF = 0; PC_ID = 0; PC_EX = 0; PC = em.getNPC();} 
        }
    }
    // Completion cycle
    if (n_op != 1)
    {
        cout << "===== Completion cycle: " << dec << cycle - 1 << " =====\n" << endl;
        cout << "Current pipeline PC state:\n{||||}\n" << endl;
        cout << "Current register values:\nPC: 0x" << hex << PC << "\nRegisters:" << endl;
        for(int i = 0; i < R.size(); i++)
        {
            string reg = "R"+to_string(i); cout << reg << ": 0x";
            int convert = R[reg];
            cout << hex << convert << endl;
        }
        cout << "\n";
    }
    
    if (m_op)// -m
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