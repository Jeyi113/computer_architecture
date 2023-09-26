#include <iostream>
#include <fstream>
#include <ostream>
#include <cstring>
#include <string>
#include <cmath>
#include <bitset>
#include <vector>
#include "Contents.h"

using namespace std;

int cpt1, cpt2, ast1, ast2, bsize, crmt; // cpt : capacity, ast : associativity, bsize : block size, crmt : cache replacement
int way1, way2, entry1, entry2, ibit1, ibit2, obit;

bool PowerOf2(int num){return (num > 0) && ((num & (num - 1)) == 0);}
vector<string> ReadFile(string file)
{
    ifstream read(file);
    string content;
    vector<string> trace;
    //1. open the file, store each contenst (convert hex to bit)
    if (read.is_open())
    {
        while(read.peek() != EOF)
        {
            getline(read, content);
            trace.push_back(content);
        }
        read.close();
    }
    else {cout << "Cannot open the file" << endl; vector<string> nothing; return nothing;}
    return trace;
}

int main(int argc, char* argv[])
{

    // 1. parameter read
    if (strcmp(argv[1], "-c") != 0 || strcmp(argv[3], "-a") != 0 || strcmp(argv[5], "-b") != 0)
    {cout << "The option template is ./runfile <-c capacity> <-a associativity> <-b block_size> <-lru or -random> <tracefile>" << endl; return -1;}
    
    /*argv[1],[2] : capacity*/
    if (PowerOf2(stoi(argv[2])) && (4 <= stoi(argv[2]) <= 1024))
    {
        cpt2 = stoi(argv[2]);
        cpt1 = cpt2/4;
    }
    else {cout << "The capacity must be the power of 2 between 4 to 1024 kB." << endl; return -1;}
    
    /*argv[3],[4] : associativity*/
    if ((PowerOf2(stoi(argv[4])) && (1 < stoi(argv[4]) && stoi(argv[4]) <= 16)) || stoi(argv[4]) == 1)
    {
        ast2 = stoi(argv[4]);
        if (ast2 <= 2){ast1 = ast2;}
        else {ast1 = ast2/4;}
    }
    else {cout << "The associativity must be the power of 2 between 1 to 16 except 1." << endl; return -1;}
    
    /*argv[5],[6] : block size*/
    if (PowerOf2(stoi(argv[6])) && (16 <= stoi(argv[6]) <= 128)){bsize = stoi(argv[6]);}
    else {cout << "The block size must be the power of 2 between 16 to 128." << endl; return -1;}
    
    /*argv[7] : Cache replacement lru / random*/
    if (strcmp(argv[7], "-lru") == 0){crmt = 0;}
    else if (strcmp(argv[7], "-random") == 0){crmt = 1;}
    else {cout << "Select the cache replacement mode between lru and random." << endl; return -1;}
    
    /*argv[8] : tracefile*/
    string filename;
    if (argc == 9){filename = argv[8];}
    else if (argc < 9){cout << "The options are not sufficient to run the program." << endl; return -1;}
    else {cout << "The options are too much to run the program." << endl; return -1;}
    
    // 2. Read File
    vector<string> trace = ReadFile(filename);
    int total = trace.size();
    if (total == 0){return -1;}
    int read = 0, write = 0, rm1 = 0, rm2 = 0, wm1 = 0, wm2 = 0, clean1 = 0, clean2 = 0, dirty1 = 0, dirty2 = 0;

    // 3. Caching
    way1 = ast1; // # of way (L1)
    way2 = ast2; // # of way (L2)
    entry1 = (cpt1*1024) / (bsize*way1); // # of L1 cache entry (# of sets)
    entry2 = (cpt2*1024) / (bsize*way2); // # of L2 cache entry (# of sets)
    ibit1 = log2(entry1); // # of index bits (L1)
    ibit2 = log2(entry2); // # of index bits (L2)
    obit = log2(bsize); // # of offset bits (L1 & L2)
    content L1[entry1][way1] = {}, L2[entry2][way2] = {};

    for (int i = 0; i < total; i++)
    {
        char mode = trace[i][0]; // R or W
        bitset<32> address(stoul(trace[i].erase(0,4), nullptr, 16)); // physical address
        string padrs = address.to_string(); unsigned int phy = stoul(padrs,nullptr,2); 
        string offset = padrs.substr(padrs.size() - obit, obit);
        string index1 = padrs.substr(padrs.size() - obit - ibit1, ibit1); int i1 = stoi(index1, nullptr, 2); 
        string index2 = padrs.substr(padrs.size() - obit - ibit2, ibit2); int i2 = stoi(index2, nullptr, 2); 
        string tag1 = padrs.substr(0, padrs.size() - obit - ibit1); int t1 = stoi(tag1, nullptr, 2);
        string tag2 = padrs.substr(0, padrs.size() - obit - ibit2); int t2 = stoi(tag2, nullptr, 2);

        bool L1miss = 1, L2miss = 1, update1 = 0, update2 = 0;
        int less1, less2; // stores recently less used
        int lessIndex1, lessIndex2; // stores the index of recently less used
        int before1, before2; // used priority before update 
        int updated1, updated2 ; // updated index
        switch (mode)
        {
        case 'R':
            read += 1;
            // L1 check
            for (int j = 0; j < way1; j++)
            {
                if (L1[i1][j].getValid() == 1 && L1[i1][j].getTag() == t1)
                { 
                    L1miss = 0;
                    before1 = L1[i1][j].getUsed();
                    updated1 = j;
                    L1[i1][j].setUsed(1);
                }
            } // L1 hit
            if(L1miss) // L1 miss -> L2 check
            {
                rm1 += 1;
                // L1 update
                less1 = 1;
                int random;
                for (int j = 0; j < way1; j++)
                {
                    if (update1 == 0 && L1[i1][j].getValid() == 0) // Valid = 0 first.
                    {
                        update1 = 1; L1[i1][j].setPadrs(phy); L1[i1][j].setTag(t1); L1[i1][j].setValid();
                        before1 = L1[i1][j].getUsed(); 
                        updated1 = j;
                        L1[i1][j].setUsed(1); 
                    }
                }
                // Find LRU
                for (int j = 0; j < way1; j++)
                {
                    if (L1[i1][j].getUsed() >= less1){less1 = L1[i1][j].getUsed(); lessIndex1 = j;}
                }

                if (update1 == 0) // Every way slot is fulled. (Valid = 1 for everything) : must choose one to evict.
                {
                    switch (crmt)
                    {
                    case 0:
                        /* lru */
                        if (L1[i1][lessIndex1].getDirty() == 1){dirty1 += 1;}
                        else {clean1 += 1;}

                        L1[i1][lessIndex1].setPadrs(phy); L1[i1][lessIndex1].setTag(t1); L1[i1][lessIndex1].setDirty(0);
                        before1 = L1[i1][lessIndex1].getUsed();
                        updated1 = lessIndex1;
                        L1[i1][lessIndex1].setUsed(1);
                        break;

                    case 1:
                        /* random */
                        random = (rand() % way1);

                        if (L1[i1][random].getDirty() == 1){dirty1 += 1;}
                        else {clean1 += 1;}

                        L1[i1][random].setPadrs(phy); L1[i1][random].setTag(t1); L1[i1][lessIndex1].setDirty(0);
                        before1 = L1[i1][random].getUsed();
                        updated1 = random;
                        L1[i1][random].setUsed(1);
                        break;
                    
                    default:
                        break;
                    }
                }

                // L2 check
                for (int j = 0; j < way2; j++)
                {
                    if (L2[i2][j].getValid() == 1 && L2[i2][j].getTag() == t2)
                    {
                        L2miss = 0;
                        before2 = L2[i2][j].getUsed();
                        updated2 = j;
                        L2[i2][j].setUsed(1);
                    }
                } // L2 hit
                if (L2miss) // L2miss -> memory
                {
                    rm2 += 1;
                    // L2 update
                    less2 = 1;
                    int random;
                    for (int j = 0; j < way2; j++)
                    {
                        if (update2 == 0 && L2[i2][j].getValid() == 0) // Valid = 0 first
                        {
                            update2 = 1; L2[i2][j].setPadrs(phy); L2[i2][j].setTag(t2); L2[i2][j].setValid();
                            before2 = L2[i2][j].getUsed();
                            updated2 = j;
                            L2[i2][j].setUsed(1);
                        }
                    }

                    // Find LRU
                    for (int j = 0; j < way2; j++)
                    {
                        if (L2[i2][j].getUsed() >= less2){less2 = L2[i2][j].getUsed(); lessIndex2 = j;}
                    }

                    if (update2 == 0) // Every way slot is fulled. (Valid = 1 for everything) : choose one to evict.
                    {
                        switch (crmt)
                        {
                        case 0:
                            /* lru */
                            if (L2[i2][lessIndex2].getDirty() == 1){dirty2 += 1;}
                            else {clean2 += 1;}

                            L2[i2][lessIndex2].setPadrs(phy); L2[i2][lessIndex2].setTag(t2); L2[i2][lessIndex2].setDirty(0);
                            before2 = L2[i2][lessIndex2].getUsed();
                            updated2 = lessIndex2;
                            L2[i2][lessIndex2].setUsed(1);
                            break;

                        case 1:
                            /* random */
                            random = (rand() % way2);

                            if (L2[i2][random].getDirty() == 1){dirty2 += 1;}
                            else {clean2 += 1;}

                            L2[i2][random].setPadrs(phy); L2[i2][random].setTag(t2); L2[i2][lessIndex2].setDirty(0);
                            before2 = L2[i2][random].getUsed();
                            updated2 = random;
                            L2[i2][random].setUsed(1);
                            break;

                        default:
                            break;
                        }
                    }
                }
                else {/*cout << "R L2 hit" << endl;*/}
            }
            else {/*cout << "R L1 hit" << endl;*/}

            // LRU update
            for (int j = 0; j < way1; j++)
            {
                if (j != updated1 && L1[i1][j].getUsed() <= before1){L1[i1][j].upUsed();}
            }
            for (int j = 0; j < way2; j++)
            {
                if (L1miss == 1 && j != updated2 && L2[i2][j].getUsed() <= before2){L2[i2][j].upUsed();}
            } 

            break;
        case 'W':
            write += 1;
            // L1 check
            for (int j = 0; j < way1; j++)
            {
                if (L1[i1][j].getValid() == 1 && L1[i1][j].getTag() == t1)
                {
                    L1miss = 0;
                    before1 = L1[i1][j].getUsed();
                    updated1 = j;
                    L1[i1][j].setUsed(1);
                    L1[i1][j].setDirty(1); // Write hit - Write back (dirty bit = 1, change it to 0 when evicted.)
                }
            } // L1 hit
            if(L1miss) // L1 miss -> L2 check
            {
                wm1 += 1;
                // L1 Update
                less1 =1;
                int random;
                for (int j = 0; j < way1; j++)
                {
                    if (update1 == 0 && L1[i1][j].getValid() == 0) // Valid = 0 first
                    {
                        update1 = 1; L1[i1][j].setPadrs(phy); L1[i1][j].setTag(t1); L1[i1][j].setValid();
                        before1 = L1[i1][j].getUsed();
                        updated1 = j;
                        L1[i1][j].setUsed(1);
                    }
                }

                // Find LRU
                for (int j = 0; j < way1; j++)
                {
                    if (L1[i1][j].getUsed() >= less1){less1 = L1[i1][j].getUsed(); lessIndex1 = j;}
                }

                if (update1 == 0) // Every way slot is fulled. (Valid = 1 for everything)
                {
                    switch (crmt)
                    {
                    case 0:
                        /* LRU */
                        if (L1[i1][lessIndex1].getDirty() == 1){dirty1 += 1;}
                        else {clean1 += 1;}

                        L1[i1][lessIndex1].setPadrs(phy); L1[i1][lessIndex1].setTag(t1); L1[i1][lessIndex1].setDirty(0);
                        before1 = L1[i1][lessIndex1].getUsed();
                        updated1 = lessIndex1;
                        L1[i1][lessIndex1].setUsed(1);
                        break;

                    case 1:
                        /* random */
                        random = (rand() % way1);
                        
                        if (L1[i1][random].getDirty() == 1){dirty1 += 1;}
                        else {clean1 += 1;}
                        
                        L1[i1][random].setPadrs(phy); L1[i1][random].setTag(t1); L1[i1][random].setDirty(0);
                        before1 = L1[i1][random].getUsed();
                        updated1 = random;
                        L1[i1][random].setUsed(1);
                        break;
                    
                    default:
                        break;
                    }
                }

                // L2 check
                for (int j = 0; j < way2; j++)
                {
                    if (L2[i2][j].getValid() == 1 && L2[i2][j].getTag() == t2)
                    {
                        L2miss = 0;
                        before2 = L2[i2][j].getUsed();
                        updated2 = j;
                        L2[i2][j].setUsed(1);
                        L2[i2][j].setDirty(1); // Write hit - Write back (dirty bit = 1, change it to 0 when evicted.)
                    }
                } // L2 hit
                if (L2miss) // L2miss -> memory
                {
                    wm2 += 1;
                    // L2 update
                    less2 = 1;
                    int random;
                    for (int j = 0; j < way2; j++)
                    {
                        if (update2 == 0 && L2[i2][j].getValid() == 0) // Valid = 0 first
                        {
                            update2 = 1; L2[i2][j].setPadrs(phy); L2[i2][j].setTag(t2); L2[i2][j].setValid();
                            before2 = L2[i2][j].getUsed();
                            updated2 = j;
                            L2[i2][j].setUsed(1);
                        }
                    }
                    
                    // Find LRU
                    for (int j = 0; j < way1; j++)
                    {
                        if (L2[i2][j].getUsed() >= less2){less2 = L2[i2][j].getUsed(); lessIndex2 = j;}
                    }

                    if (update2 == 0) // Every way slot is fulled. (Valid = 1 for everything)
                    {
                        switch (crmt)
                        {
                        case 0:
                            /* LRU */
                            if (L2[i2][lessIndex2].getDirty() == 1){dirty2 += 1;}
                            else {clean2 += 1;}

                            L2[i2][lessIndex2].setPadrs(phy); L2[i2][lessIndex2].setTag(t2); L2[i2][lessIndex2].setDirty(0);
                            before2 = L2[i2][lessIndex2].getUsed();
                            updated2 = lessIndex2;
                            L2[i2][lessIndex2].setUsed(1);
                            break;
                        
                        case 1:
                            /* random */
                            random = (rand() % way2);
            
                            if (L2[i2][random].getDirty() == 1){dirty2 += 1;}
                            else {clean2 += 1;}

                            L2[i2][random].setPadrs(phy); L2[i2][random].setTag(t2); L2[i2][random].setDirty(0);
                            before2 = L2[i2][random].getUsed();
                            updated2 = random;
                            L2[i2][random].setUsed(1);
                            break;
                        
                        default:
                            break;
                        }
                    }
                }
                else {/*cout << "W L2 hit" << endl;*/}
            }
            else {/*cout << "W L1 hit" << endl;*/}

            // LRU update
            for (int j = 0; j < way1; j++)
            {
                if (j != updated1 && L1[i1][j].getUsed() <= before1){L1[i1][j].upUsed();}
            }
            for (int j = 0; j < way2; j++)
            {
                if (L1miss == 1 && j != updated2 && L2[i2][j].getUsed() <= before2){L2[i2][j].upUsed();}
            }

            break;

        default:
            break;
        }
    }

    // 4. File Write
    /*L1 Capacity, way, L2 Capacity, way, Block Size, Total, Read, Write accesses, L1 Read miss, L2 Read miss, L1 Write miss, L2 Write miss, L1 Read miss rate, L2 Read miss rate, L1 Write miss rate, L2 Write miss rate, L1 Clean eviction, L2 Clean eviction, L1 dirty eviction, L2 dirty eviction*/
    auto it = filename.end();
    filename.erase(it-4, it);
    string info = "_" + to_string(cpt2) + "_" + to_string(way2) + "_" + to_string(bsize) + ".out";
    filename.append(info);

    double l1rmr = (static_cast<double>(rm1) / total);
    double l2rmr = (static_cast<double>(rm2) / rm1);
    double l1wmr = (static_cast<double>(wm1) / total);
    double l2wmr = (static_cast<double>(wm2) / wm1);

    ofstream wf(filename);
    if(wf.is_open())
    {
        wf << "-- General Stats --\nL1 Capacity: " << cpt1 << "\nL1 way: " << way1 << "\nL2 Capacity: " << cpt2 << "\nL2 way: " << way2 << "\nBlock Size: " << bsize << endl;
        wf << "Total accesses: " << total << "\nRead accesses: " << read << "\nWrite accesses: " << write << endl;
        wf << "L1 Read misses: " << rm1 << "\nL2 Read misses: " << rm2 << "\nL1 Write misses: " << wm1 << "\nL2 Write misses: " << wm2 << endl;
        wf << "L1 Read miss rate: " << l1rmr*100 << "%\nL2 Read miss rate: " << l2rmr*100 << "%\nL1 Write miss rate: " << l1wmr*100 << "%\nL2 Write miss rate: " << l2wmr*100 << "%" << endl;
        wf << "L1 Clean eviction: " << clean1 << "\nL2 Clean eviction: " << clean2 << "\nL1 Dirty eviction: " << dirty1 << "\nL2 Dirty eviction: " << dirty2 << endl;
        wf.close();
    }

    return 0;
}