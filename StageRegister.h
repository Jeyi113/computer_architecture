#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <bitset>

using namespace std;

class StageRegister
{
    protected:
        int RegDst = -1;
        int ALUOp1 = -1;
        int ALUOp2 = -1;
        int ALUSrc = -1;
        int Brch = -1;
        int Jump = -1;
        int MemRead = -1;
        int MemWrite = -1;
        int RegWrite = -1;
        int MemtoReg = -1;
        int noop = 0;
        unsigned int op;
        unsigned int rs;
        unsigned int rt;
        unsigned int rd;
        unsigned int sh;
        unsigned int fn;
        signed int imm;
    public:
        StageRegister();
        void setNoop(int np);
        void setRegDst(int rdst);
        void setALUOp1(int ao1);
        void setALUOp2(int ao2);
        void setALUSrc(int as);
        void setBrch(int b);
        void setJump(int j);
        void setMemRead(int mr);
        void setMemWrite(int mw);
        void setRegWrite(int rw);
        void setMemtoReg(int mtr);
        void setOp(bitset<6> b0);
        void setRs(bitset<5> b1);
        void setRt(bitset<5> b2);
        void setRd(bitset<5> b3);
        void setSh(bitset<5> b4);
        void setImm(bitset<32> b5);
        void setFn(bitset<6> b6);
        // There's difference in accessing those Control signal in each stage register, so get function is implemented seperately.
        int getRegDst();
        int getALUOp1();
        int getALUOp2();
        int getALUSrc();
        int getBrch();
        int getJump();
        int getMemRead();
        int getMemWrite();
        int getRegWrite();
        int getMemtoReg();
        unsigned int getOp();
        unsigned int getRs();
        unsigned int getRt();
        unsigned int getRd();
        unsigned int getSh();
        signed int getImm();
        unsigned int getFn();
};

class IF_ID : public StageRegister
{
    private:
        bitset<32> instruction;
        unsigned int NPC;
        long data1;
        long data2;
        int forward;
    public:
        IF_ID();
        void setInst(bitset<32> inst);
        void setNPC(unsigned int npc);
        void setData1(long d1);
        void setData2(long d2);
        void setForward(int f);
        bitset<32> getInst();
        unsigned int getNPC();
        long getData1();
        long getData2();
        int getForward();
};

class ID_EX : public StageRegister
{
    private:
        unsigned int NPC;
        unsigned int BR_TARGET;
        long shift;
        long data1;
        long data2;
    public:
        ID_EX();
        void setNPC(unsigned int npc);
        void setBt(unsigned int bt);
        void setShift(bitset<32> b8);
        void setData1(long d1);
        void setData2(long d2);
        unsigned int getNPC();
        unsigned int getBt();
        long getShift();
        long getData1();
        long getData2();
};

class EX_MEM : public StageRegister
{
    private:
        unsigned int NPC;
        unsigned int op;
        unsigned int dst; // destination register (rd or rt)
        unsigned int BR_TARGET;
        long ALU_OUT;
        long shift;
    public:
        EX_MEM();
        void setNPC(unsigned int npc);
        void setOp(unsigned int o);
        void setDst(unsigned int d);
        void setBt(unsigned int bt);
        void setALU(long alu);
        void setShift(long sft);
        unsigned int getNPC();
        unsigned int getOp();
        unsigned int getDst();
        unsigned int getBt();
        long getALU();
        long getShift();
};

class MEM_WB : public StageRegister
{
    private:
        unsigned int NPC;
        unsigned int dst; // destination reg num
        unsigned int BR_TARGET;
        unsigned long ALU_OUT;
        long Mem_Out;
        long shift;
    public:
        MEM_WB();
        void setNPC(unsigned int npc);
        void setDst(unsigned int d);
        void setBt(unsigned int bt);
        void setALU(long alu);
        void setMemOut(long m);
        void setShift(long sft);
        unsigned int getNPC();
        unsigned int getDst();
        unsigned int getBt();
        long getALU();
        long getMem();
        long getShift();
};

StageRegister::StageRegister(){}
void StageRegister::setNoop(int np){noop = np;} // np = 1 if noop operation, else np = 0
void StageRegister::setRegDst(int rdst){RegDst = rdst;}
void StageRegister::setALUOp1(int ao1){ALUOp1 = ao1;}
void StageRegister::setALUOp2(int ao2){ALUOp2 = ao2;}
void StageRegister::setALUSrc(int as){ALUSrc = as;}
void StageRegister::setBrch(int b){Brch = b;}
void StageRegister::setJump(int j){Jump = j;}
void StageRegister::setMemRead(int mr){MemRead = mr;}
void StageRegister::setMemWrite(int mw){MemWrite = mw;}
void StageRegister::setRegWrite(int rw){RegWrite = rw;}
void StageRegister::setMemtoReg(int mtr){MemtoReg = mtr;}
void StageRegister::setOp(bitset<6> b0){op = b0.to_ulong();}
void StageRegister::setRs(bitset<5> b1){rs = b1.to_ulong();}
void StageRegister::setRt(bitset<5> b2){rt = b2.to_ulong();}
void StageRegister::setRd(bitset<5> b3){rd = b3.to_ulong();}
void StageRegister::setSh(bitset<5> b4){sh = b4.to_ulong();}
void StageRegister::setImm(bitset<32> b5){imm = static_cast<int32_t>(b5.to_ulong());}
void StageRegister::setFn(bitset<6> b6){fn = b6.to_ulong();}
int StageRegister::getRegDst(){if(noop){return -1;} return RegDst;}
int StageRegister::getALUOp1(){if(noop){return -1;} return ALUOp1;}
int StageRegister::getALUOp2(){if(noop){return -1;} return ALUOp2;}
int StageRegister::getALUSrc(){if(noop){return -1;} return ALUSrc;}
int StageRegister::getBrch(){if(noop){return -1;} return Brch;}
int StageRegister::getJump(){if(noop){return -1;} return Jump;}
int StageRegister::getMemRead(){if(noop){return -1;} return MemRead;}
int StageRegister::getMemWrite(){if(noop){return -1;} return MemWrite;}
int StageRegister::getRegWrite(){if(noop){return -1;} return RegWrite;}
int StageRegister::getMemtoReg(){if(noop){return -1;} return MemtoReg;}
unsigned int StageRegister::getOp(){if(noop){return 0;} return op;}
unsigned int StageRegister::getRs(){if(noop){return 0;} return rs;}
unsigned int StageRegister::getRt(){if(noop){return 0;} return rt;}
unsigned int StageRegister::getRd(){if(noop){return 0;} return rd;}
unsigned int StageRegister::getSh(){if(noop){return 0;} return sh;}
signed int StageRegister::getImm(){if(noop){return 0;} return imm;}
unsigned int StageRegister::getFn(){if(noop){return 0;} return fn;}

IF_ID::IF_ID(){}
void IF_ID::setInst(bitset<32> inst){instruction = inst;}
void IF_ID::setNPC(unsigned int npc){NPC = npc;}
void IF_ID::setData1(long d1){data1 = d1;}
void IF_ID::setData2(long d2){data2 = d2;}
void IF_ID::setForward(int f){forward = f;} // 0 : no forwarding , 1 : data1, 2 : data2
bitset<32> IF_ID::getInst(){if(noop){return 0;} return instruction;}
unsigned int IF_ID::getNPC(){if(noop){return 0;} return NPC;}
long IF_ID::getData1(){if(noop){return 0;} return data1;}
long IF_ID::getData2(){if(noop){return 0;} return data2;}
int IF_ID::getForward(){if(noop){return 0;} return forward;}

ID_EX::ID_EX(){}
void ID_EX::setNPC(unsigned int npc){NPC = npc;}
void ID_EX::setBt(unsigned int bt){BR_TARGET = bt;}
void ID_EX::setShift(bitset<32> b8){shift = b8.to_ulong();}
void ID_EX::setData1(long d1){data1 = d1;}
void ID_EX::setData2(long d2){data2 = d2;}
unsigned int ID_EX::getNPC(){if(noop){return 0;} return NPC;}
unsigned int ID_EX::getBt(){if(noop){return 0;} return BR_TARGET;}
long ID_EX::getShift(){if(noop){return 0;} return shift;}
long ID_EX::getData1(){if(noop){return 0;} return data1;}
long ID_EX::getData2(){if(noop){return 0;} return data2;}

EX_MEM::EX_MEM(){}
void EX_MEM::setNPC(unsigned int npc){NPC = npc;}
void EX_MEM::setOp(unsigned int o){op = o;}
void EX_MEM::setDst(unsigned int d){dst = d;}
void EX_MEM::setBt(unsigned int bt){BR_TARGET = bt;}
void EX_MEM::setALU(long alu){ALU_OUT = alu;}
void EX_MEM::setShift(long sft){shift = sft;}
unsigned int EX_MEM::getNPC(){if(noop){return 0;} return NPC;}
unsigned int EX_MEM::getOp(){if(noop){return 0;} return op;}
unsigned int EX_MEM::getDst(){if(noop){return 0;} return dst;}
unsigned int EX_MEM::getBt(){if(noop){return 0;} return BR_TARGET;}
long EX_MEM::getShift(){if(noop){return 0;} return shift;}
long EX_MEM::getALU(){if(noop){return 0;} return ALU_OUT;}

MEM_WB::MEM_WB(){}
void MEM_WB::setNPC(unsigned int npc){NPC = npc;}
void MEM_WB::setDst(unsigned int d){dst = d;}
void MEM_WB::setBt(unsigned int bt){BR_TARGET = bt;}
void MEM_WB::setALU(long alu){ALU_OUT = alu;}
void MEM_WB::setMemOut(long m){Mem_Out = m;}
void MEM_WB::setShift(long sft){shift = sft;}
unsigned int MEM_WB::getNPC(){if(noop){return 0;} return NPC;}
unsigned int MEM_WB::getDst(){if(noop){return 0;} return dst;}
unsigned int MEM_WB::getBt(){if(noop){return 0;} return BR_TARGET;}
long MEM_WB::getALU(){if(noop){return 0;} return  ALU_OUT;}
long MEM_WB::getMem(){if(noop){return 0;} return Mem_Out;}
long MEM_WB::getShift(){if(noop){return 0;} return shift;}