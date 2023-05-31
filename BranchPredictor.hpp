

#include <bits/stdc++.h>

using namespace std;


// 00-strongly not taken
// 01-weakly not taken
// 10 - weakly taken
// 11 - strongly taken

struct BranchPredictor {
    virtual bool predict(uint32_t pc) = 0;
    virtual void update(uint32_t pc, bool taken) = 0;
};

struct SaturatingBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> table;
    SaturatingBranchPredictor(int value) : table(1 << 14, value) {}

    bool predict(uint32_t pc) {
        // your code here
        bitset<2> c=table[(pc<<18>>18)];
        if(c[1]==1){
            return true;
        }
        return false;
    }

    void update(uint32_t pc, bool taken) {
        // your code here
        bitset<2> c=table[pc<<18>>18];
        if(c[0]==1 && c[1]==1){
            if(!taken){
                table[pc<<18>>18][0]=0;
            }
        }else if(c[0]==0 && c[1]==1){
            if(taken){
                table[pc<<18>>18][0]=1;
            }else{
                table[pc<<18>>18][0]=1;
                table[pc<<18>>18][1]=0;

            }
        }else if(c[0]==1 && c[1]==0){
            if(taken){
                table[pc<<18>>18][0]=0;
                table[pc<<18>>18][1]=1;
            }else{
                table[pc<<18>>18][0]=0;
            }
        }else{
            if(taken){
                table[pc<<18>>18][0]=1;
            }
        }
    }
};

struct BHRBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> bhrTable;
    std::bitset<2> bhr;
    BHRBranchPredictor(int value) : bhrTable(1 << 2, value), bhr(value) {}

    bool predict(uint32_t pc) {
        // your code here
        int ind=(int)(bhr.to_ulong());
        bitset<2> c=bhrTable[ind];
        if(c[1]==1){
            return true;
        }
        return false;

        return false;
    }

    void update(uint32_t pc, bool taken) {
        // your code here
        int ind=(int)(bhr.to_ulong());
        bitset<2> c=bhrTable[ind];
        if(c[0]==1 && c[1]==1){
            if(!taken){
                bhrTable[ind][0]=0;
            }
        }else if(c[0]==0 && c[1]==1){
            if(taken){
                bhrTable[ind][0]=1;
            }else{
                bhrTable[ind][0]=1;
                bhrTable[ind][1]=0;

            }
        }else if(c[0]==1 && c[1]==0){
            if(taken){
                bhrTable[ind][0]=0;
                bhrTable[ind][1]=1;
            }else{
                bhrTable[ind][0]=0;
            }
        }else{
            if(taken){
                bhrTable[ind][0]=1;
            }
        }
        bhr[1]=bhr[0];
        if(taken){
            bhr[0]=1;
        }else{
            bhr[0]=0;
        }
    }
};

struct SaturatingBHRBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> bhrTable;
    std::bitset<2> bhr;
    std::vector<std::bitset<2>> table;
    std::vector<std::bitset<2>> combination;
    SaturatingBHRBranchPredictor(int value, int size) : bhrTable(1 << 2, value), bhr(value), table(1 << 14, value), combination(size, value) {
        assert(size <= (1 << 16));

    }

    bool predict(uint32_t pc) {
        // your code here
        bitset<2> c=table[(pc<<18>>16 + bhr.to_ulong() )];
        if(c[1]==1){
            return true;
        }
        return false;


        return false;
    }

    void update(uint32_t pc, bool taken) {
        // your code here
        bitset<2> c=table[(pc<<18>>16 + bhr.to_ulong() )];
        if(c[0]==1 && c[1]==1){
            if(!taken){
                table[pc<<18>>18][0]=0;
            }
        }else if(c[0]==0 && c[1]==1){
            if(taken){
                table[(pc<<18>>16 + bhr.to_ulong() )][0]=1;
            }else{
                table[(pc<<18>>16 + bhr.to_ulong() )][0]=1;
                table[(pc<<18>>16 + bhr.to_ulong() )][1]=0;

            }
        }else if(c[0]==1 && c[1]==0){
            if(taken){
                table[(pc<<18>>16 + bhr.to_ulong() )][0]=0;
                table[(pc<<18>>16 + bhr.to_ulong() )][1]=1;
            }else{
                table[(pc<<18>>16 + bhr.to_ulong() )][0]=0;
            }
        }else{
            if(taken){
                table[(pc<<18>>16 + bhr.to_ulong() )][0]=1;
            }
        }
        bhr[1]=bhr[0];
        if(taken){
            bhr[0]=1;
        }else{
            bhr[0]=0;
        }
    }
};

