#include <fstream>
#include <iostream>
#include <cstring>
#include <sstream>
#include <iostream>
#include <vector>

#include "cache.h"

#define CACHE_SETS 16
#define MEM_SIZE 4096 // bytes
#define CACHE_WAYS 1
#define BLOCK_SIZE 1 // bytes per block
#define DM 0
#define SA 1


using namespace std;

struct trace {
    bool MemR;
    bool MemW;
    int adr;
    int data;
};

/*
Either implement your memory_controller here or use a separate .cpp/.c file for memory_controller and all the other functions inside it (e.g., LW, SW, Search, Evict, etc.)
*/


int main(int argc,
    char* argv[]) // the program runs like this: ./program <filename> <mode>
{
    // input file (i.e., test.txt)
    string filename = argv[1];

    // mode for replacement policy
    //int cacheType;

    ifstream fin;

    // opening file
    fin.open(filename.c_str());
    if (!fin) { // making sure the file is correctly opened
        cout << "Error opening " << filename << endl;
        exit(1);
    }


    // reading the text file
    string line;
    vector<trace> myTrace;
    int TraceSize = 0;
    string s1, s2, s3, s4;

    //read trace and put each line of trace in vector
    while (getline(fin, line)) {
        //get trace line
        stringstream ss(line);
        getline(ss, s1, ',');
        getline(ss, s2, ',');
        getline(ss, s3, ',');
        getline(ss, s4, ',');

        //push back empty trace
        myTrace.push_back(trace());  

        //assign values to empty trace
        myTrace[TraceSize].MemR = stoi(s1); 
        myTrace[TraceSize].MemW = stoi(s2);
        myTrace[TraceSize].adr = stoi(s3);
        myTrace[TraceSize].data = stoi(s4);
        //cout<<myTrace[TraceSize].MemW << endl;
        
        //tracks which trace we are on
        TraceSize += 1;
    }

    auto mc = memory_controller();

    // counters for miss rate
    int cacheLW = 0;
    int cacheSW = 0;

    int clock = 0;

    int traceCounter = 0;
    bool cur_MemR;
    bool cur_MemW;
    int cur_adr;
    int cur_data;
    // this is the main loop of the code
    //go back through trace and perform operations
    while (traceCounter < TraceSize) {
            cur_MemR = myTrace[traceCounter].MemR;
            cur_MemW = myTrace[traceCounter].MemW;
            cur_data = myTrace[traceCounter].data;
            cur_adr = myTrace[traceCounter].adr;
            traceCounter += 1;
            if (cur_MemR == 1)
                cacheLW += 1;
            else if (cur_MemW == 1)
                cacheSW += 1;
      
        mc.clock_cycle(cur_MemR, cur_MemW, cur_data, cur_adr);
        clock += 1;
    }

    //while (status < 1) { // to make sure that the last access is also done
    //    status = mc.clock_cycle(cur_MemR, cur_MemW, cur_data, cur_adr);
    //    clock += 1;
    //}
    cout << "L1 accesses: " << mc.get_L1_access_count() << endl;
    cout << "L2 accesses: " << mc.get_L2_access_count() << endl;

    cout << "L1 misses: " << mc.get_L1_miss_count() << endl;
    cout << "L2 misses: " << mc.get_L2_miss_count() << endl;

    float L1_miss_rate = mc.get_L1_miss_count() / (float)mc.get_L1_access_count();
    float L2_miss_rate = mc.get_L2_miss_count() / (float)mc.get_L2_access_count();

    float AAT = 1 + (L1_miss_rate * (8 + (L2_miss_rate * (100))));

    // printing the final result
    cout << "(" << L1_miss_rate << "," << L2_miss_rate << "," << AAT << ")" << endl;


    // closing the file
    fin.close();

    return 0;
}