#pragma once
#include <iostream>

class content
{
    private:
        unsigned int padrs = 0;
        bool dirty = 0;
        int used = 0; // priority (recently used has low used num, high priority)
        int valid = 0;
        int tag = 0;
    public:
        content();
        void setPadrs(unsigned int adrs);
        void setTag(int tg);
        void setDirty(bool d);
        void setUsed(int num);
        void upUsed();
        void setValid();
        unsigned int getPadrs();
        bool getDirty();
        int getUsed();
        int getValid();
        int getTag();
};

content::content(){}
void content::setPadrs(unsigned int adrs){padrs = adrs;}
void content::setTag(int tg){tag = tg;}
void content::setDirty(bool d){dirty = d;}
void content::setUsed(int num){used = num;}
void content::upUsed(){used += 1;}
void content::setValid(){valid = 1;}
unsigned int content::getPadrs(){return padrs;}
bool content::getDirty(){return dirty;}
int content::getUsed(){return used;}
int content::getValid(){return valid;}
int content::getTag(){return tag;}