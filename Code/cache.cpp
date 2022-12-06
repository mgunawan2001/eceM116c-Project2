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

    cout << "L1: ";
    for (int i = 0; i < 16; i++) {
        cout << L1Cache[i].data << ", ";
    }
    cout << endl;

    cout << "L1 tags: ";
    for (int i = 0; i < 16; i++) {
        cout << L1Cache[i].tag << ", ";
    }
    cout << endl << endl;

    cout << "L2: ";
    for (int j = 0; j < 128; j++) {
        cout << L2Cache[j].data << ", ";
    }
    cout << endl;

    cout << "L2 tags: ";
    for (int j = 0; j < 128; j++) {
        cout << L2Cache[j].tag << ", ";
    }
    cout << endl;

    cout << "L2 LRU: ";
    for (int j = 0; j < 128; j++) {
        cout << L2Cache[j].lru_position << ", ";
    }

    cout << endl << endl;
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
void memory_controller::insert_L1(int& cur_data, int cur_adr) {
    int da_cur_tag = cur_adr >> 4;
    int da_cur_index = cur_adr & 0xF;
    int input_adr;

    //if index already contains data, move data to L2
    if (L1Cache[da_cur_index].tag != -1) {
        
        input_adr = (L1Cache[da_cur_index].tag << 4) + da_cur_index;
        insert_L2(L1Cache[da_cur_index].data, input_adr);
    }
            
    //put new data into index
    L1Cache[da_cur_index].tag = da_cur_tag;
    L1Cache[da_cur_index].data = cur_data;
    cur_data = L1Cache[da_cur_index].data;
}

void memory_controller::insert_L2(int& cur_data, int cur_adr) {
    int sa_cur_tag = cur_adr >> 4;
    int sa_cur_set_index = (cur_adr & 0xF) * 8;

    cout << "tag: " << sa_cur_tag << endl;

    int updated = -1;

    

    for (int i = sa_cur_set_index; i < sa_cur_set_index + 8; i++) {
        /*if (L2Cache[sa_cur_set_index + i].tag == sa_cur_tag) {
            L2Cache[sa_cur_set_index + i].data = cur_data;
            L2Cache[sa_cur_set_index + i].tag = sa_cur_tag;
            L2Cache[sa_cur_set_index + i].lru_position = 7;
            updated = i;
            break;
        }*/
        if (L2Cache[sa_cur_set_index + i].tag == -1 or L2Cache[sa_cur_set_index + i].lru_position == 0) {
            L2Cache[sa_cur_set_index + i].data = cur_data;
            L2Cache[sa_cur_set_index + i].tag = sa_cur_tag;
            L2Cache[sa_cur_set_index + i].lru_position = 7;

            updated = i; //for lru replacement
            break;
        }
    }

    for (int i = sa_cur_set_index; i < sa_cur_set_index + 8; i++) {
        //if tag != 1: there is value in cache index i
        //i!=update: don't update the value we just put in
        if (L2Cache[i].tag != -1 && i != updated) {
            L2Cache[i].lru_position--;
        }                 
    }
}

/*Direct Associate LW*/
void memory_controller::load_word_L1(int& cur_data, int cur_adr) {
        L1AccessCount++;

        //get tag and index from address
        int da_cur_tag = cur_adr >> 4;
        int da_cur_index = (cur_adr & 0xF)%16;

        cout << "here1 adr: " << cur_adr << " data: " << cur_data << endl;

        //if cache hit get data and return
        if (L1Cache[da_cur_index].tag == da_cur_tag) { 
            cout << "lw L1 hit" << endl;
            L1HitCount++;            

            cur_data = L1Cache[da_cur_index].data;   //get data if found in cache
        }

        //if cache miss go to L2
        else {
            cout << "lw L1 miss, ";
            L1MissCount++;

            load_word_L2(cur_data, cur_adr);
        }

}

/*Set Associate LW*/
void memory_controller::load_word_L2(int& cur_data, int cur_adr) {
    L2AccessCount++;

    int sa_cur_tag = cur_adr >> 4;
    int sa_cur_set_index = (cur_adr & 0xF) * 8;

    int hit = 0;

    int L1exists = 0;
    int temp_tag, temp_data;

    // Checks for cache hit
    for (int i = 0; i < 8; i++) {  
        //cache hit!
        if (L2Cache[sa_cur_set_index + i].tag == sa_cur_tag) {
            cout << "lw L2: hit " << endl;
            L2HitCount++;
            

            //get data
            cur_data = L2Cache[sa_cur_set_index + i].data;
            insert_L1(cur_data, cur_adr);
            
            hit = 1;
            break;
        }
    }

    // No hit was found - Cache miss. Evict LRU and replace with data from memory
    if (hit == 0) {
        cout << "lw L2: miss " << endl;
        L2MissCount++;

        //get data from memory
        Mem[cur_adr] = cur_data;
        insert_L1(Mem[cur_adr], cur_adr);

        
    }
}

/*Direct associative SW*/
void memory_controller::store_word_L1(int& cur_data, int cur_adr) {
        L1AccessCount++;

        //get tag and index from address
        int da_cur_tag = cur_adr >> 4;
        int da_cur_index = cur_adr & 0xF;

        //update value in main mem
        Mem[cur_adr] = cur_data;

        //cout << "here: " << cur_data << endl;
        //cache hit. 
        if (L1Cache[da_cur_index].tag == da_cur_tag) {
            cout << "sw L1: hit" << endl;
            L1HitCount++;            

            L2Cache[da_cur_index].data = cur_data;
            L2Cache[da_cur_index].tag = da_cur_tag;
        }

        else {
            cout << "sw L1: miss, ";
            L1MissCount++;

            store_word_L2(cur_data,cur_adr);
        }
        cur_data = 0;
        
}

/*Set associative SW*/
void memory_controller::store_word_L2(int& cur_data, int cur_adr) {
    L2AccessCount++;

    int hit = 0;
    int sa_cur_tag = cur_adr >> 4;
    int sa_cur_set_index = (cur_adr & 0xF) * 8; //goes to index of first element in set 

    for (int i = 0; i < 8; i++) {
        //cache hit
        if (L2Cache[sa_cur_set_index + i].tag == sa_cur_tag) {
            cout << "sw L2: hit" << endl;
            L2HitCount++;

            insert_L1(L2Cache[sa_cur_set_index + i].data, cur_adr);
        
            hit = 1;            
            break;
        }
    }

    if (hit == 0) {
        cout << "sw L2: miss " << endl;
        L2MissCount++;
    }
}

void memory_controller::printCache() {
    cout << endl << "L1: ";
    for (auto& i : L1Cache) {
        cout << i.data << ", ";
    }
    cout << endl;

    cout << "L2: ";
    for (auto& i : L2Cache) {
        cout << i.data << ", ";
    }
    cout << endl;
}