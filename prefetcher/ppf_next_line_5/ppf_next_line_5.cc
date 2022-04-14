#include <iostream>
#include <algorithm>
#include <array>
#include <map>
#include <math.h>
#include <ctime>
#include <fstream>

#include "cache.h"

//int cache_hit_count_1 = 0;
//int cache_hit_count_2 = 0;

constexpr int PREFETCH_DEGREE = 5;    //degree of prefetch

constexpr int TRANSFER_BUFFER_ENTRIES = 2048;                // entries of transfer buffer

constexpr int NUM_FEATURES_USED = 4;                    // defines the number of features used by us in this perceptron (including bias)

constexpr int FT_DEGREE_ENTRIES = pow(2, ceil(log2(PREFETCH_DEGREE)));        // entries of degree feature table

constexpr int FT_IP_FOLD_1_BITS = 10;                    // define the final folding bits of IP as feature 1 (e.g. 10-bit folded IP)
constexpr int FT_IP_FOLD_1_ENTRIES = 1 << FT_IP_FOLD_1_BITS;        // entries of IP folding 1 feature table

constexpr int FT_PF_FOLD_1_BITS = 10;                    // define the final folding bits of PF as feature 1 (e.g. 10-bit folded PF)
constexpr int FT_PF_FOLD_1_ENTRIES = 1 << FT_PF_FOLD_1_BITS;        // number of entries of PF folding 1 feature table

#define INCREMENT(X) (X = (X==15) ? X : X+1)
#define DECREMENT(X) (X = (X==-16) ? X : X-1)
#define THRESHOLD(X) (X = (X>0) ? 1 : 0)
#define MOVE_PTR_UP(X) (X = (X == (TRANSFER_BUFFER_ENTRIES-1) ? 0 : X+1))

struct transfer_buff_entry {
    uint64_t pf_addr = 0;
    int ip_fold_1 = 0;
    int pf_fold_1 = 0;
    int pf_degree = 0;
    int valid = 0;
    int ppf_prediction = 0;                            // indicates the prediction that was made by Perceptron
};

struct lookahead_entry {
    uint64_t address = 0;
    int degree = 0;            // degree remaining
    uint64_t ip = 0;            // IP
};

int ft_degree[FT_DEGREE_ENTRIES];                        // array to declare degree feature table
int ft_ip_fold_1[FT_IP_FOLD_1_ENTRIES];                    // array to declare IP folding 1 feature table
int ft_pf_fold_1[FT_PF_FOLD_1_ENTRIES];                    // array to declare PF folding 1 feature table
int bias;

transfer_buff_entry trans_buff[TRANSFER_BUFFER_ENTRIES];
int ptr_to_trans_buff = 0;

int ppf_decision(int, uint64_t, uint64_t);

int func_ip_fold_1(uint64_t);
int func_pf_fold_1(uint64_t);

void retrain_ppf(transfer_buff_entry *, int, int);

std::map<CACHE *, lookahead_entry> lookahead;

void CACHE::prefetcher_initialize() {
    std::cout << NAME << " PPF over next line prefetcher with " << NUM_FEATURES_USED << " features" << std::endl;
    //std::cout << NAME << " PPF over next line prefetcher" << std::endl;
    bias = 1;
    srand(time(0));

    for (int i = 0; i < FT_DEGREE_ENTRIES; i++) {
        ft_degree[i] = (rand() % 6) - 2;        //randomly initialize weights between -2 and 3
    }

    for (int i = 0; i < FT_IP_FOLD_1_ENTRIES; i++) {
        ft_ip_fold_1[i] = (rand() % 6) - 2;        //randomly initialize weights between -2 and 3
    }

    for (int i = 0; i < FT_PF_FOLD_1_ENTRIES; i++) {
        ft_pf_fold_1[i] = (rand() % 6) - 2;                // initialize weights between -2 and 3
    }
}

void CACHE::prefetcher_cycle_operate() {
    int ppf_prediction;
    auto[old_pf_address, degree, ip] = lookahead[this];
    // If a lookahead is active
    if (degree > 0) {
        auto pf_address = old_pf_address + 1;
        // If the next step would exceed the degree or run off the page, stop
        if (virtual_prefetch || (pf_address >> LOG2_PAGE_SIZE) == (old_pf_address >> LOG2_PAGE_SIZE)) {
            ppf_prediction = ppf_decision(degree - 1, ip, pf_address);

            // check the MSHR occupancy to decide if we're going to prefetch to this level or not
            //bool success = prefetch_line(pf_address, true, 0);
            if (ppf_prediction) {
                bool success = prefetch_line(pf_address,
                                             (get_occupancy(0, pf_address) < get_size(0, pf_address) / 2), 0);
                if (success) {
                    MOVE_PTR_UP(ptr_to_trans_buff);
                    if (trans_buff[ptr_to_trans_buff].valid == 1) {
                        retrain_ppf(trans_buff, ptr_to_trans_buff, 0);
                    }

                    trans_buff[ptr_to_trans_buff].pf_degree = degree;
                    trans_buff[ptr_to_trans_buff].ip_fold_1 = func_ip_fold_1(ip);
                    trans_buff[ptr_to_trans_buff].pf_addr = pf_address;
                    trans_buff[ptr_to_trans_buff].pf_fold_1 = func_pf_fold_1(pf_address);

                    trans_buff[ptr_to_trans_buff].valid = 1;
                    trans_buff[ptr_to_trans_buff].ppf_prediction = ppf_prediction;

                    lookahead[this] = {pf_address, degree - 1, ip};
                }
            } else {
                MOVE_PTR_UP(ptr_to_trans_buff);
                if (trans_buff[ptr_to_trans_buff].valid == 1) {
                    retrain_ppf(trans_buff, ptr_to_trans_buff, 0);
                }

                trans_buff[ptr_to_trans_buff].pf_addr = pf_address;
                trans_buff[ptr_to_trans_buff].ip_fold_1 = func_ip_fold_1(ip);
                trans_buff[ptr_to_trans_buff].pf_fold_1 = func_pf_fold_1(pf_address);
                trans_buff[ptr_to_trans_buff].pf_degree = degree;
                trans_buff[ptr_to_trans_buff].valid = 1;
                trans_buff[ptr_to_trans_buff].ppf_prediction = ppf_prediction;

                lookahead[this] = {pf_address, degree - 1, ip};
            }
            // If we fail, try again next cycle
        } else {
            lookahead[this] = {};
        }
    }
}

uint32_t
CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in) {
    //uint64_t pf_addr = addr + (1 << LOG2_BLOCK_SIZE);
    uint64_t cl_addr = addr >> LOG2_BLOCK_SIZE;
    lookahead[this] = {cl_addr, PREFETCH_DEGREE, ip};
    //prefetch_line(ip, addr, pf_addr, true, 0);

    if (cache_hit) {
        for (int i = 0; i < TRANSFER_BUFFER_ENTRIES; i++) {
            if ((trans_buff[i].pf_addr == cl_addr) && (trans_buff[i].valid == 1)) {
                //cache_hit_count_2++;
                retrain_ppf(trans_buff, i, 1);
            }
        }
    }
    return metadata_in;
}

uint32_t
CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr,
                             uint32_t metadata_in) {
    return metadata_in;
}

void CACHE::prefetcher_final_stats() {
}


int ppf_decision(int degree, uint64_t ip, uint64_t pf) {
    int sum = 0;
    int feature[NUM_FEATURES_USED];

    feature[0] = bias;

    int index_degree = degree;
    feature[1] = ft_degree[index_degree];

    int index_ip_fold_1 = func_ip_fold_1(ip);
    feature[2] = ft_ip_fold_1[index_ip_fold_1];

    int index_pf_fold_1 = func_pf_fold_1(pf);
    feature[3] = ft_pf_fold_1[index_pf_fold_1];

    for (int i = 0; i < NUM_FEATURES_USED; i++) {
        sum += feature[i];
    }

    return THRESHOLD(sum);
}

int func_ip_fold_1(uint64_t ip) {
    uint64_t new_ip = ip >> LOG2_BLOCK_SIZE;            // e.g., we have 58 bits as 6 LSB are block offset
    int folds = ceil((64 - LOG2_BLOCK_SIZE) / FT_IP_FOLD_1_BITS);    // ceil((64 - 6)/10) = ceil(58/10) = ceil(5.8) = 6

    int mask = 0;
    int lsb_bits = 0;
    int folded_ip = 0;
    for (int i = 0; i < folds; i++) {
        mask = (1 << FT_IP_FOLD_1_BITS) - 1;
        lsb_bits = mask & new_ip;
        new_ip = new_ip >> FT_IP_FOLD_1_BITS;
        folded_ip = lsb_bits ^ folded_ip;
    }

    return folded_ip;
}

int func_pf_fold_1(uint64_t pf) {
    uint64_t new_pf = pf >> LOG2_BLOCK_SIZE;            // e.g., we have 58 bits as 6 LSB are block offset
    int folds = ceil((64 - LOG2_BLOCK_SIZE) / FT_PF_FOLD_1_BITS);    // ceil((64 - 6)/10) = ceil(58/10) = ceil(5.8) = 6

    int mask = 0;
    int lsb_bits = 0;
    int folded_pf = 0;
    for (int i = 0; i < folds; i++) {
        mask = (1 << FT_PF_FOLD_1_BITS) - 1;
        lsb_bits = mask & new_pf;
        new_pf = new_pf >> FT_PF_FOLD_1_BITS;
        folded_pf = lsb_bits ^ folded_pf;
    }

    return folded_pf;
}

void retrain_ppf(transfer_buff_entry *trans_buff, int ptr_to_trans_buff, int useful) {
    int index_ip_fold_1;
    int index_pf_fold_1;
    int index_degree;

    index_ip_fold_1 = trans_buff[ptr_to_trans_buff].ip_fold_1;
    index_pf_fold_1 = trans_buff[ptr_to_trans_buff].pf_fold_1;
    index_degree = trans_buff[ptr_to_trans_buff].pf_degree -
                   1;    // index_degree is 1 less than the actual degree, e.g., for degree=1, it should index entry number 0

    if (useful) {
        INCREMENT(ft_ip_fold_1[index_ip_fold_1]);
        INCREMENT(ft_pf_fold_1[index_pf_fold_1]);
        INCREMENT(ft_degree[index_degree]);
        INCREMENT(bias);
    } else {
        DECREMENT(ft_ip_fold_1[index_ip_fold_1]);
        DECREMENT(ft_pf_fold_1[index_pf_fold_1]);
        DECREMENT(ft_degree[index_degree]);
        DECREMENT(bias);
    }
    trans_buff[ptr_to_trans_buff].valid = 0;
}