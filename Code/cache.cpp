#include "cache.h"

#include <bitset>
using namespace std;

// Constructor that initialized the cache
memory_controller::memory_controller() {
    status = 1;
    daMissCount = 0;
    saMissCount = 0;
    hitCount = 0;

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

// Representative of one clock cycle. Takes an action based on the "status" of
// the cache along with the passed in parameters.
int memory_controller::clock_cycle(bool cur_MemR, bool cur_MemW, int& cur_data,
    int cur_adr) {
    // Stall for a cycle until ready
    if (status < 1) {
        if (status == 0) {
            cur_data = Mem[cur_adr];
        }
        ++status;
    }
    // Memory ready (LW)
    else if (cur_MemR == 1) {
        load_word_L1(cur_data, cur_adr);
        cout << endl;
    }
    // Memory write (SW)
    else if (cur_MemW == 1) {
        store_word_L1(cur_data, cur_adr);
        cout << endl;
    }

    return status;
}

// Returns direct associate miss count
int memory_controller::get_da_miss_count() const {
    return daMissCount;
}

// Returns set associate miss count
int memory_controller::get_sa_miss_count() const {
    return saMissCount;
}

// Made for debugging. Returns hit count.
int memory_controller::get_hit_count() const {
    return hitCount;
}

/*Direct Associate LW*/
void memory_controller::load_word_L1(int& cur_data, int cur_adr) {
    
    //if (cacheType == DM) {
        //get tag and index from address
        int da_cur_tag = cur_adr >> 4;
        int da_cur_index = cur_adr & 0xF;

        //if cache hit get data and return
        if (L1Cache[da_cur_index].tag == da_cur_tag) { 
            cur_data = L1Cache[da_cur_index].data;   //get data if found in cache
            hitCount++;
            status = 1;

            cout << "lw L1 hit, ";
        }

        //if cache miss go to L2
        else {
            daMissCount++;
            cout << "lw L1 miss, ";
            load_word_L2(cur_data, cur_adr);
            
            status = -3;
            
        }
    //}

}

/*Set Associate LW*/
void memory_controller::load_word_L2(int& cur_data, int cur_adr) {

    int sa_cur_tag = cur_adr >> 4;
    int sa_cur_set_index = (cur_adr & 0xF) * 8;

    int temp_lru = 0;  //temp_lru = old_lru
    int updated = -1;
    int hit = 0;


    // Checks for cache hit
    for (int i = 0; i < 8; i++) {

        //cache hit!
        if (L2Cache[sa_cur_set_index + i].tag == sa_cur_tag) {

            //update LRU
            temp_lru = L2Cache[sa_cur_set_index + i].lru_position;
            L2Cache[sa_cur_set_index + i].lru_position = 7;

            //get data
            cur_data = L2Cache[sa_cur_set_index + i].data;
            //cout << "data: " << Cache[cur_set_index + i].data << endl;

            updated = sa_cur_set_index + i; //for lru replacement
            hitCount++;
            hit = 1;
            status = 1;
            //                std::cerr << "HIT  --> ";

            cout << "lw L2: hit " << endl;
        }
    }

    // No hit was found - Cache miss. Evict LRU and replace with data from memory
    if (hit == 0) {
        saMissCount++;
        updated = -1;
        //            std::cerr << "MISS --> ";
        cout << "lw L2: miss " << endl;

        
        for (int i = sa_cur_set_index; i < sa_cur_set_index + 8; i++) {
            if (L2Cache[i].tag == -1 or L2Cache[i].lru_position == 0) {
                //load data from memory into cache
                L2Cache[i].data = Mem[cur_adr];
                L2Cache[i].tag = sa_cur_tag;
                L2Cache[i].lru_position = 7;
                //get data to return
                cur_data = L2Cache[i].data;


                status = -3;
                updated = i; //for lru replacement
                break;
            }
        }
    }

    // Updates LRUs
    for (int i = sa_cur_set_index; i < sa_cur_set_index + 4; i++) {
        //if tag != 1: there is value in cache index i
        //Cache[i].lru_position > temp_lru: update newer(>) values to be older 
        //i!=update: don't update the value we just put in
        if (L2Cache[i].tag != -1 && L2Cache[i].lru_position > temp_lru && i != updated) {
            L2Cache[i].lru_position--;
        }
        //}

        //        std::cerr << "LW--> Current Address: " << cur_adr
        //                  << "     Current Data: " << cur_data << "     Set-Index: "
        //                  << cur_set_index / 4 << "-" << updated % 4
        //                  << "     Current Tag: " << cur_tag << std::endl;
    }
}

// Handles the store word process for each cache cacheType.
void memory_controller::store_word_L1(int& cur_data, int cur_adr) {
    /*Direct associative SW*/
    //if (cacheType == 0) {
        //get tag and index from address
        int da_cur_tag = cur_adr >> 4;
        int da_cur_index = cur_adr & 0xF;

        //cache hit. 
        if (L1Cache[da_cur_index].tag == da_cur_tag) {
            L2Cache[da_cur_index].data = cur_data;

            // Regardless of hit or miss
            Mem[cur_adr] = cur_data;
            status = 1;
            cur_data = 0;
            cout << "sw L1: hit, ";
        }

        else {
            cout << "sw L1: miss, ";
            store_word_L2(cur_data, cur_adr);
        }
        
    //}
}

/*Set associative SW*/
void memory_controller::store_word_L2(int& cur_data, int cur_adr) {
    int hit = 0;
    int sa_cur_tag = cur_adr >> 4;
    int sa_cur_set_index = (cur_adr & 0xF) * 8; //goes to index of first element in set 
    //cout << endl << "set #: " << (cur_adr & 0xF)%8 << endl;
    //cout << "tag: " << sa_cur_tag << endl;
    //check each value in set
    for (int i = 0; i < 8; i++) {
        
        //cout << endl << "   - tag " << i << ": " << L2Cache[sa_cur_set_index + i].tag << endl;
        //cache hit
        if (L2Cache[sa_cur_set_index + i].tag == sa_cur_tag) {
            L2Cache[sa_cur_set_index + i].data = cur_data;
            hit = 1;
            cout << "sw L2: hit" << endl;
            //cout << "   tag: " << L2Cache[sa_cur_set_index + i].tag << endl;
            //cout << "   sa_cur_set_index + i: " << sa_cur_set_index + i << endl;
            break;
        }
    }

    if (hit == 0) {
        cout << "sw L2: miss " << endl;
        //cout << "index 71: " << L2Cache[71].data << endl;
    }

    // Regardless of hit or miss
    Mem[cur_adr] = cur_data;
    status = 1;
    cur_data = 0;

    //printCache();
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