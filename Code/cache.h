#ifndef cache
#define cache

#include <iostream>

#define MEM_SIZE 4096 // bytes
#define CACHE_SETS 16
#define DM 0
#define SA 1

class memory_controller {

public:
    explicit memory_controller();

    //~memory_controller();

    void clock_cycle(bool cur_MemR, bool cur_MemW, int& cur_data, int cur_adr);

    int get_L1_miss_count() const;
    int get_L2_miss_count() const;

    int get_L1_hit_count() const;
    int get_L2_hit_count() const;

    int get_L1_access_count() const;
    int get_L2_access_count() const;

private:
    struct cache_element {
        int tag; 
        int lru_position; 
        int data; 
    };


    //declare count variables
    int L1MissCount, L2MissCount;
    int L1HitCount, L2HitCount;
    int L1AccessCount,  L2AccessCount;

    //initialize Mem and Caches
    int Mem[MEM_SIZE]{};
    cache_element L1Cache[16]{}; //16 elements
    cache_element L2Cache[128]{};  //16*8=128 elements

    //load word functions
    void load_word_L1(int& cur_data, int cur_adr);
    void load_word_L2(int& cur_data, int cur_adr);

    //store word functions
    void store_word_L1(int& cur_data, int cur_adr);
    void store_word_L2(int& cur_data, int cur_adr);

    //function for updating L1 and L2 
    void insert_L1(int& cur_data, int cur_adr, int swap);
};



#endif 