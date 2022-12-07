#include "cache.h"

#include <bitset>
using namespace std;

// Constructor that initialized the cache
memory_controller::memory_controller() {
    L1MissCount = 0;
    L2MissCount = 0;

    L1HitCount = 0;
    L2HitCount = 0;

    L1AccessCount = 0;
    L2AccessCount = 0;

    //initialize cache with empty elements
    for (int i = 0; i < 16; i++) {
        L1Cache[i].tag = -1; // -1 indicates that the tag value is invalid. We don't use a separate VALID bit.
        L1Cache[i].lru_position = 0;  // 0 means it is the lowest position
        L1Cache[i].data = 0;
    }

    for (int j = 0; j < 128; j++) {
        L2Cache[j].tag = -1; // -1 indicates that the tag value is invalid. We don't use a separate VALID bit.
        L2Cache[j].lru_position = 0;  // 0 means it is the lowest position
        L2Cache[j].data = 0;
    }
}

// Deconstructor
//memory_controller::~memory_controller() = default;

void memory_controller::clock_cycle(bool cur_MemR, bool cur_MemW, int& cur_data,
    int cur_adr) {

    // Memory ready (LW)
    if (cur_MemR == 1) {
        load_word_L1(cur_data, cur_adr);
    }
    // Memory write (SW)
    else if (cur_MemW == 1) {
        store_word_L1(cur_data, cur_adr);
    }
    //
    //    cout << "L1: ";
    //    for (int i = 0; i < 16; i++) {
    //        cout << L1Cache[i].data << ", ";
    //    }
    //    cout << endl;
    //
    //    cout << "L1 tags: ";
    //    for (int i = 0; i < 16; i++) {
    //        cout << L1Cache[i].tag << ", ";
    //    }
    //    cout << endl << endl;
    //
    //    cout << "L2: ";
    //    for (int j = 0; j < 128; j++) {
    //        cout << L2Cache[j].data << ", ";
    //    }
    //    cout << endl;
    //
    //    cout << "L2 tags: ";
    //    for (int j = 0; j < 128; j++) {
    //        cout << L2Cache[j].tag << ", ";
    //    }
    //    cout << endl;
    //
    //    cout << "L2 LRU: ";
    //    for (int j = 0; j < 128; j++) {
    //        cout << L2Cache[j].lru_position << ", ";
    //    }
    //
    //    cout << endl << endl;
}

// Returns direct associate miss count
int memory_controller::get_L1_miss_count() const {
    return L1MissCount;
}

// Returns set associate miss count
int memory_controller::get_L2_miss_count() const {
    return L2MissCount;
}

// Made for debugging. Returns hit count.
int memory_controller::get_L1_hit_count() const {
    return L1HitCount;
}

int memory_controller::get_L2_hit_count() const {
    return L2HitCount;
}

int memory_controller::get_L1_access_count() const {
    return L1AccessCount;
}

int memory_controller::get_L2_access_count() const {
    return L2AccessCount;
}

//inserts new data into L1 and moves old data to L2. Also updates mem
void memory_controller::insert_L1(int& cur_data, int cur_adr, int swap) {
    //cur_data contains data that was just hit and needs to be put in L1
    //cur_adr is L2 adress that was hit. We want to put its data in L1 in the same index and move L1 data down to L2

    int L2_tag = cur_adr >> 4;
    int L2_set_index = cur_adr & 0xF;
    
    int L1_tag = L1Cache[L2_set_index].tag;
    //int L1_set_index = L2_set_index;   //L2 data will be put in L2[L2_index]

    int temp_lru = 0;
    int updated = -1;

    //if index in L1 contains data, move data to L2
    if (L1Cache[L2_set_index].tag != -1) {
        //swap L1 and L2
        if (swap == 1) {
            //find and get data from L2
            for (int i = L2_set_index; i < L2_set_index + 8; i++) {
                if (L2Cache[L2_set_index + i].tag == L2_tag) {
                    //keep track of the LRU of item we are replacing so we can move newer elements forward
                    temp_lru = L2Cache[L2_set_index + i].lru_position;

                    //put L1 data in L2
                    L2Cache[L2_set_index + i].data = L1Cache[L2_set_index].data;
                    L2Cache[L2_set_index + i].tag = L1_tag;
                    L2Cache[L2_set_index + i].lru_position = 7;

                    updated = i; //for lru replacement
                    break;
                }
            }
        }
        //move L1 data into LRU position
        else if (swap == 0) {            
            //insert_L2(L1Cache[L2_index].data, L1_adr, L2_adr);
            for (int i = L2_set_index; i < L2_set_index + 8; i++) {
                //LRU replacement
                if (L2Cache[L2_set_index + i].tag == -1 or L2Cache[L2_set_index + i].lru_position == 0) {
                    //keep track of the LRU of item we are replacing so we can move newer elements forward
                    temp_lru = L2Cache[L2_set_index + i].lru_position;
                    
                    L2Cache[L2_set_index + i].data = L1Cache[L2_set_index].data;
                    L2Cache[L2_set_index + i].tag = L1_tag;
                    L2Cache[L2_set_index + i].lru_position = 7;

                    updated = i; //for lru replacement
                    break;
                }
            }
        }

        for (int i = L2_set_index; i < L2_set_index + 8; i++) {
            //if tag != 1: there is value in cache index i
            //i!=update: don't update the value we just put in
            //L2Cache[i].lru_position > temp_lru: move newer elements one position forward
            if (L2Cache[i].tag != -1 && i != updated && L2Cache[i].lru_position > temp_lru) {
                L2Cache[i].lru_position--;
            }
        }
        
    }
            
    //put new data into index
    L1Cache[L2_set_index].tag = L2_tag;
    L1Cache[L2_set_index].data = cur_data;
    cur_data = L1Cache[L2_set_index].data;
}

/*Direct Associate LW*/
void memory_controller::load_word_L1(int& cur_data, int cur_adr) {
        L1AccessCount++;

        //get tag and index from address
        int L1_tag = cur_adr >> 4;
        int L1_index = (cur_adr & 0xF)%16;

        //cout << "here1 adr: " << cur_adr << " data: " << cur_data << endl;

        //if cache hit get data and return
        if (L1Cache[L1_index].tag == L1_tag) { 
            //cout << "lw L1 hit" << endl;
            L1HitCount++;            

            cur_data = L1Cache[L1_index].data;   //get data if found in cache
        }

        //if cache miss go to L2
        else {
            //cout << "lw L1 miss, ";
            L1MissCount++;

            load_word_L2(cur_data, cur_adr);
        }

}

/*Set Associate LW*/
void memory_controller::load_word_L2(int& cur_data, int cur_adr) {
    L2AccessCount++;

    int L2_tag = cur_adr >> 4;
    int L2_set_index = (cur_adr & 0xF) * 8;

    int hit = 0;

    // Checks for cache hit
    for (int i = 0; i < 8; i++) {  
        //cache hit!
        if (L2Cache[L2_set_index + i].tag == L2_tag) {
            //cout << "lw L2: hit " << endl;
            L2HitCount++;
            
            //get data
            cur_data = L2Cache[L2_set_index + i].data;
            insert_L1(cur_data, cur_adr, 1);
            
            hit = 1;
            break;
        }
    }

    // No hit was found - Cache miss. Evict LRU and replace with data from memory
    if (hit == 0) {
        //cout << "lw L2: miss " << endl;
        L2MissCount++;

        //get data from memory and insert into L1
        insert_L1(Mem[cur_adr], cur_adr, 0);
    }
}

/*Direct associative SW*/
void memory_controller::store_word_L1(int& cur_data, int cur_adr) {
        L1AccessCount++;

        //get tag and index from address
        int L1_tag = cur_adr >> 4;
        int L1_index = cur_adr & 0xF;

        //update value in main mem
        Mem[cur_adr] = cur_data;

        //cout << "here: " << cur_data << endl;
        //cache hit. 
        if (L1Cache[L1_index].tag == L1_tag) {
            //cout << "sw L1: hit" << endl;
            L1HitCount++;            

            L2Cache[L1_index].data = cur_data;
            L2Cache[L1_index].tag = L1_tag;
        }

        else {
            //cout << "sw L1: miss, ";
            L1MissCount++;

            store_word_L2(cur_data,cur_adr);
        }
        cur_data = 0;
        
}

/*Set associative SW*/
void memory_controller::store_word_L2(int& cur_data, int cur_adr) {
    L2AccessCount++;

    int hit = 0;
    int L2_tag = cur_adr >> 4;
    int L2_set_index = (cur_adr & 0xF) * 8; //goes to index of first element in set 

    for (int i = 0; i < 8; i++) {
        //cache hit
        if (L2Cache[L2_set_index + i].tag == L2_tag) {
            //cout << "sw L2: hit" << endl;
            L2HitCount++;

            insert_L1(L2Cache[L2_set_index + i].data, cur_adr, 1);
        
            hit = 1;            
            break;
        }
    }

    if (hit == 0) {
        //cout << "sw L2: miss " << endl;
        L2MissCount++;
    }
}
