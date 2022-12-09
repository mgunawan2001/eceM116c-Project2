#include "cache.h"

#include <bitset>
using namespace std;

// Constructor that initialized the cache
memory_controller::memory_controller() {
    L1MissCount = 0;
    L2MissCount = 0;
    victimMissCount = 0;

    L1HitCount = 0;
    L2HitCount = 0;
    victimHitCount = 0;

    L1AccessCount = 0;
    L2AccessCount = 0;
    victimAccessCount = 0;

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

    //victim cache
    for (int k = 0; k < 4; k++) {
        VictimCache[k].tag = -1; // -1 indicates that the tag value is invalid. We don't use a separate VALID bit.
        VictimCache[k].lru_position = 0;  // 0 means it is the lowest position
        VictimCache[k].data = 0;
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
    
        /*cout << "L1: ";
        for (int i = 0; i < 16; i++) {
            cout << L1Cache[i].data << ", ";
        }
        cout << endl;
    
        cout << "L1 tags: ";
        for (int i = 0; i < 16; i++) {
            cout << L1Cache[i].tag << ", ";
        }
        cout << endl << endl;

        cout << "Victim: ";
        for (int i = 0; i < 4; i++) {
            cout << VictimCache[i].data << ", ";
        }
        cout << endl;

        cout << "Victim tags: ";
        for (int i = 0; i < 4; i++) {
            cout << VictimCache[i].tag << ", ";
        }
        cout << endl;

        cout << "Victim LRU: ";
        for (int j = 0; j < 4; j++) {
            cout << VictimCache[j].lru_position << ", ";
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
    
        cout << endl << endl;*/
}

// Returns direct associate miss count
int memory_controller::get_L1_miss_count() const {
    return L1MissCount;
}

// Returns set associate miss count
int memory_controller::get_L2_miss_count() const {
    return L2MissCount;
}

int memory_controller::get_victim_miss_count() const {
    return victimMissCount;
}

// Made for debugging. Returns hit count.
int memory_controller::get_L1_hit_count() const {
    return L1HitCount;
}

int memory_controller::get_L2_hit_count() const {
    return L2HitCount;
}

int memory_controller::get_victim_hit_count() const {
    return victimHitCount;
}

int memory_controller::get_L1_access_count() const {
    return L1AccessCount;
}

int memory_controller::get_L2_access_count() const {
    return L2AccessCount;
}

int memory_controller::get_victim_access_count() const {
    return victimAccessCount;
}

//inserts new data into L1 and moves old data to L2. Also updates mem
void memory_controller::insert_L1_L2(int& cur_data, int cur_adr, int hit) {
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
        //hit L1 and L2
        if (hit == 1) {
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
        else if (hit == 0) {            
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

//inserts new data into L1 and moves old data to L2. Also updates mem
void memory_controller::insert_L1_victim(int& cur_data, int cur_adr, int victimIndex, int hit) {
    //hit=0 miss everywhere
    //hit=1 victim cache hit
    //hit=2 L2 hit

    int temp_lru, temp_lru2, temp_data;
    int updated, updated2;

    int L1_tag = cur_adr >> 4; //get tag of hit in L1 format
    int L1_index = (cur_adr & 0xF) % 16;

    int L2_tag = cur_adr >> 4; //get tag of hit in L1 format
    int L2_index = (cur_adr & 0xF) * 8;

    if (hit == 0) {
        //cur_adr = L1 to put memory data in
        
        //if there is already data in L1 index, put data in victim cache
        if (L1Cache[L1_index].tag != -1) {
            //check if there is space to put data in victim cache
            for (int i = 0; i < 4; i++) {
                //check if there is somewhere to put data in victim cache
                if (VictimCache[i].tag == -1 or VictimCache[i].lru_position == 0) {
                    temp_lru = VictimCache[i].lru_position;
                    updated = i;

                    //if index already has data, move it to L2. lru_pos==0
                    if (VictimCache[i].tag != -1) {
                        L2_tag = VictimCache[i].tag >> 4;
                        L2_index = (VictimCache[i].tag & 0xF) * 8;

                        //put data in L2
                        for (int j = L2_index; j < L2_index + 8; j++) {
                            //LRU replacement
                            if (L2Cache[j].tag == -1 || L2Cache[j].lru_position == 0) {
                                //keep track of the LRU of item we are replacing so we can move newer elements forward
                                temp_lru2 = L2Cache[j].lru_position;
                                updated2 = j; //for lru replacement


                                L2Cache[j].data = VictimCache[i].data;
                                L2Cache[j].tag = (VictimCache[i].tag >> 4);
                                L2Cache[j].lru_position = 7;
                                
                                for (int i = L2_index; i < L2_index + 8; i++) {
                                    //if tag != 1: there is value in cache index i
                                    //i!=update: don't update the value we just put in
                                    //L2Cache[i].lru_position > temp_lru: move newer elements one position forward
                                    if (L2Cache[i].tag != -1 && i != updated2 && L2Cache[i].lru_position > temp_lru2) {
                                        L2Cache[i].lru_position--;
                                    }
                                }

                                break;
                            }
                        }                        
                    }

                    VictimCache[i].data = L1Cache[L1_index].data;
                    VictimCache[i].tag = (L1Cache[L1_index].tag << 4) + L1_index;
                    VictimCache[i].lru_position = 3;

                    //update Victim Cache LRU
                    for (int i = 0; i < 4; i++) {
                        //if tag != 1: there is value in cache index i
                        //i!=update: don't update the value we just put in
                        //L2Cache[i].lru_position > temp_lru: move newer elements one position forward
                        if (VictimCache[i].tag != -1 && i != updated && VictimCache[i].lru_position > temp_lru) {
                            VictimCache[i].lru_position--;
                        }
                    }
                    break;
                }
            }
        }

        L1Cache[L1_index].data = cur_data;
        L1Cache[L1_index].tag = cur_adr >> 4;
    }

    //swap L1 and victim
    else if (hit == 1) {
        temp_lru = VictimCache[victimIndex].lru_position;
        updated = victimIndex;

        //update victim cache
        temp_data = VictimCache[victimIndex].data;  //data in VictimCache is cur_data bc it was hit
        VictimCache[victimIndex].data = L1Cache[L1_index].data;
        VictimCache[victimIndex].tag = (L1Cache[L1_index].tag << 4) + L1_index;
        VictimCache[victimIndex].lru_position = 3;

        //update Victim Cache LRU
        for (int i = 0; i < 4; i++) {
            //if tag != 1: there is value in cache index i
            //i!=update: don't update the value we just put in
            //L2Cache[i].lru_position > temp_lru: move newer elements one position forward
            if (VictimCache[i].tag != -1 && i != updated && VictimCache[i].lru_position > temp_lru) {
                VictimCache[i].lru_position--;
            }
        }

        L1Cache[L1_index].data = cur_data;
        L1Cache[L1_index].tag = L1_tag;
    }

    if (hit == 2) {

        //cur_adr = L1 to put memory data in

        //if there is already data in L1 index, put data in victim cache
        if (L1Cache[L1_index].tag != -1) {
            //check if there is space to put data in victim cache
            for (int i = 0; i < 4; i++) {
                //check if there is somewhere to put data in victim cache
                if (VictimCache[i].tag == -1 or VictimCache[i].lru_position == 0) {

                    //if index already has data, move it to L2. lru_pos==0
                    if (VictimCache[i].tag != -1) {
                        temp_lru2 = L2Cache[L2_index].lru_position;
                        updated2 = L2_index;


                        //put victim data in L2
                        L2Cache[L2_index].data = VictimCache[i].data;
                        L2Cache[L2_index].tag = VictimCache[i].tag >> 4;
                        L2Cache[L2_index].lru_position = 7;

                        //update L2 LRU
                        for (int i = L2_index; i < L2_index + 8; i++) {
                            //if tag != 1: there is value in cache index i
                            //i!=update: don't update the value we just put in
                            //L2Cache[i].lru_position > temp_lru: move newer elements one position forward
                            if (L2Cache[i].tag != -1 && i != updated2 && L2Cache[i].lru_position > temp_lru2) {
                                L2Cache[i].lru_position--;
                            }
                        }
                    }

                    updated = i;
                    temp_lru = VictimCache[i].lru_position;
                    VictimCache[i].data = L1Cache[L1_index].data;
                    VictimCache[i].tag = (L1Cache[L1_index].tag << 4) + L1_index;
                    VictimCache[i].lru_position = 3;

                    //update Victim Cache LRU
                    for (int i = 0; i < 4; i++) {
                        //if tag != 1: there is value in cache index i
                        //i!=update: don't update the value we just put in
                        //L2Cache[i].lru_position > temp_lru: move newer elements one position forward
                        if (VictimCache[i].tag != -1 && i != updated && VictimCache[i].lru_position > temp_lru) {
                            VictimCache[i].lru_position--;
                        }
                    }

                    L1Cache[L1_index].data = cur_data;
                    L1Cache[L1_index].tag = L1_tag;

                    

                    break;
                }
            }
        }

        L1Cache[L1_index].data = cur_data;
        L1Cache[L1_index].tag = cur_adr >> 4;
    }
}

/*Direct Associate LW*/
void memory_controller::load_word_L1(int& cur_data, int cur_adr) {
        L1AccessCount++;

        //get tag and index from address
        int L1_tag = cur_adr >> 4;
        int L1_index = (cur_adr & 0xF)%16;

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
            load_word_victim(cur_data, cur_adr);
        }

}

/*Victim LW*/
void memory_controller::load_word_victim(int& cur_data, int cur_adr) {
    victimAccessCount++;

    int hit = 0;

    //cout << "here1 adr: " << cur_adr << " data: " << cur_data << endl;

    //if cache hit get data and return
    for (int i = 0; i < 4; i++) {
        if (VictimCache[i].tag == cur_adr) {
            //cout << "lw Victim hit" << endl;
            victimHitCount++;
            hit = 1;
            insert_L1_victim(cur_data, cur_adr, i, 1);

            break;
        }
    }


    //if cache miss go to L2
    if (hit != 1) {
        //cout << "lw Victim miss ";
        victimMissCount++;
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
            insert_L1_victim(cur_data, cur_adr, i, 2);
            
            hit = 1;
            break;
        }
    }

    // No hit was found - Cache miss. Evict LRU and replace with data from memory
    if (hit == 0) {
        //cout << "lw L2: miss " << endl;
        L2MissCount++;


        //get data from memory and insert into L1
        insert_L1_victim(Mem[cur_adr], cur_adr, -1, 0);
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

            store_word_victim(cur_data,cur_adr);
        }
        cur_data = 0;
        
}

/*Victim Cache SW*/
void memory_controller::store_word_victim(int& cur_data, int cur_adr) {
    victimAccessCount++;

    int hit = 0;
    int victim_tag = cur_adr;

    for (int i = 0; i < 4; i++) {
        //cache hit
        if (L2Cache[i].tag == victim_tag) {
            //cout << "sw Victim: hit" << endl;
            victimHitCount++;

            insert_L1_victim(L2Cache[i].data, cur_adr, i, 1);

            hit = 1;
            break;
        }
    }

    if (hit == 0) {
        //cout << "sw Victim: miss ";
        victimMissCount++;
        store_word_L2(cur_data, cur_adr);
    }
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

            insert_L1_victim(L2Cache[L2_set_index + i].data, cur_adr, -1, 0);
        
            hit = 1;            
            break;
        }
    }

    if (hit == 0) {
        //cout << "sw L2: miss " << endl;
        L2MissCount++;
    }
}
