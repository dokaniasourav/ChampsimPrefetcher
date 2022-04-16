#include <algorithm>
#include <array>
#include <vector>
#include <map>
#include <math.h>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sys/stat.h>
#include "cache.h"


constexpr int PREFETCH_DEGREE = 3;
constexpr int TRANSFER_BUFFER_ENTRIES = 2048;                // entries of transfer buffer
constexpr int NUM_FEATURES_USED = 3;                         // defines the number of features used by us in this perceptron (including bias)
constexpr int FT_DEGREE_ENTRIES = 4;    // entries of degree feature table
constexpr int FT_IP_FOLD_1_BITS = 10;                       // define the final folding bits of IP as feature 1 (e.g. 10-bit folded IP)
constexpr int FT_IP_FOLD_1_ENTRIES = 1 << FT_IP_FOLD_1_BITS;// entries of IP folding 1 feature table

#define INCREMENT(X)    ((X) = ((X) == 15) ? (X) : (X) + 1)
#define DECREMENT(X)    ((X) = ((X) ==-16) ? (X) : (X) - 1)
#define THRESHOLD(X)    ((X) = ((X) >   0) ? 1 : 0)
#define MOVE_PTR_UP(X)  ((X) = ((X) == (TRANSFER_BUFFER_ENTRIES-1) ? 0 : (X) + 1))

struct tracker_entry {
    uint64_t ip = 0;              // the IP we're tracking
    uint64_t last_cl_addr = 0;    // the last address accessed by this IP
    int64_t last_stride = 0;      // the stride between the last two addresses accessed by this IP
    uint64_t last_used_cycle = 0; // use LRU to evict old IP trackers
};

struct lookahead_entry {
    uint64_t address = 0;
    int64_t stride = 0;
    int degree = 0; // degree remaining
    uint64_t ip = 0;              // the IP we're tracking
};

int ft_degree[FT_DEGREE_ENTRIES];                       // array to declare degree feature table
int ft_ip_fold_1[FT_IP_FOLD_1_ENTRIES];                 // array to declare IP folding 1 feature table
int bias;

struct transfer_buff_entry {
    uint64_t pf_addr = 0;
    int ip_fold_1 = 0;
    int pf_degree = 0;
    int valid = 0;
    int ppf_prediction = 0;                            // indicates the prediction that was made by Perceptron
};
transfer_buff_entry trans_buff[TRANSFER_BUFFER_ENTRIES];

uint64_t cache_operate_count = 0;
uint64_t cycle_operate_count = 0;
uint64_t cache_hit_count = 0;

uint64_t total_training_count = 0;
uint64_t useful_training_count = 0;
uint64_t total_prediction_count = 0;
uint64_t true_prediction_count = 0;


// uint64_t pf_requested = 0, pf_issued = 0, pf_useful = 0, pf_useless = 0, pf_fill = 0;
// Above vars are used in cache.h to record the pf_data

#define REC_TB_SIZE 2048
struct recorder_str {
    uint64_t total_training;
    uint64_t useful_training;
    uint64_t total_prediction;
    uint64_t true_prediction;
    uint64_t total_prefetch;
    uint64_t useful_prefetch;
    uint64_t cache_operate;
    uint64_t cache_hit;
};
int record_table_ind = 0;
uint64_t next_cycle_update = 0;
recorder_str record_table[REC_TB_SIZE];
#define CYCLE_UPDATE_INTERVAL 1000000

int ptr_to_trans_buff = 0;
int ppf_decision(int, uint64_t);
int func_ip_fold_1(uint64_t);
void retrain_ppf(transfer_buff_entry *, int, int);

string dir_name;
unordered_map<uint64_t, vector<int>> inverted_address;

constexpr std::size_t
TRACKER_SETS = 256;
constexpr std::size_t
TRACKER_WAYS = 4;
std::map<CACHE *, lookahead_entry> lookahead;
std::map<CACHE *, std::array < tracker_entry, TRACKER_SETS * TRACKER_WAYS>> trackers;
extern string trace_name;

void CACHE::prefetcher_initialize() {

    std::cout << NAME << " PPF over IP-based stride prefetcher" << std::endl;

    /******* Initialization Phase of Cache prefetcher code *********/
    srand(time(0));
    bias = 1;
    for (int i = 0; i < FT_DEGREE_ENTRIES; i++) {
        ft_degree[i] = (rand() % 6) - 2;                // initialize weights between -2 and 3
    }
    for (int i = 0; i < FT_IP_FOLD_1_ENTRIES; i++) {
        ft_ip_fold_1[i] = (rand() % 6) - 2;                // initialize weights between -2 and 3
    }
    /******* Initial data writing to a file in new directory ************/



    /********************* ALL FILE OPERATIONS **************************/
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "./data/ppf_ip-stride_%Y%m%d%H%M%S_");
    size_t f_ind = trace_name.find_last_of('/');
    if (f_ind != string::npos) {
        dir_name = oss.str() + trace_name.substr(f_ind+1, 3);
    } else {
        dir_name = oss.str() + std::to_string(rand() % 10000);
    }
    std::cout << NAME << " -- Directory Name = " << dir_name << std::endl;
    struct stat sst = {0};
    if (stat(dir_name.c_str(), &sst) == -1) {
        mkdir(dir_name.c_str(), 0777);
    }
    string file_name = dir_name + "/Weights_initial_" + NAME + ".txt";

    ofstream my_file;
    my_file.open(file_name);
    my_file << "Bias: " << bias << std::endl << std::endl;
    my_file << "Feature Table 1: Degree" << std::endl;
    for (int i = 0; i < FT_DEGREE_ENTRIES; i++) {
        ft_degree[i] = (rand() % 6) - 2;                // initialize weights between -2 and 3
        my_file << "i: " << i << "\tw: " << ft_degree[i] << std::endl;
    }
    my_file << std::endl;
    my_file << "Feature Table 2: IP_Fold_1" << std::endl;
    for (int i = 0; i < FT_IP_FOLD_1_ENTRIES; i++) {
        ft_ip_fold_1[i] = (rand() % 6) - 2;                // initialize weights between -2 and 3
        my_file << "i: " << i << "\tw: " << ft_ip_fold_1[i] << std::endl;
    }
    my_file << std::endl;
    my_file << "Printing trans buff:" << std::endl;
    for (int i = 0; i < TRANSFER_BUFFER_ENTRIES; i++) {
        my_file << "Entry number: " << i << "\tAddress: " << trans_buff[i].pf_addr << "\tValid: " << trans_buff[i].valid
                << "\tDegree: " << trans_buff[i].pf_degree << std::endl;
    }
    my_file << std::endl;
    my_file.close();
    /****************** End of the writing file code *****************************/
}

void CACHE::prefetcher_cycle_operate() {
    int ppf_prediction;
    cycle_operate_count++;
    if(cycle_operate_count > next_cycle_update) {
        next_cycle_update += CYCLE_UPDATE_INTERVAL;
        if(record_table_ind <  REC_TB_SIZE) {
            record_table[record_table_ind] = {total_training_count, useful_training_count,
                                              total_prediction_count, true_prediction_count,
                                                pf_requested, pf_useful,
                                              cache_operate_count, cache_hit_count};
            record_table_ind++;
        }
    }
    // If a lookahead is active
    auto[old_pf_address, stride, degree, ip] = lookahead[this];

    if (degree > 0) {
        auto pf_address = old_pf_address + stride;
        // If the next step would exceed the degree or run off the page, stop
        // check MSHR occupancy, decide if we're going to prefetch to this level or not
        if (virtual_prefetch || (pf_address >> LOG2_PAGE_SIZE) == (old_pf_address >> LOG2_PAGE_SIZE)) {
            // entry is -1 of degree....e.g. for degree=1, entry accessed is 0
            ppf_prediction = ppf_decision(degree - 1, ip);
            bool success = true;
            if(ppf_prediction) {
                true_prediction_count++;
                success = prefetch_line(pf_address, (get_occupancy(0, pf_address) < get_size(0, pf_address) / 2), 0);
            }
            if (success) {
                MOVE_PTR_UP(ptr_to_trans_buff);
                if (trans_buff[ptr_to_trans_buff].valid == 1) {
                    // Coming back to it must mean that its useless
                    retrain_ppf(trans_buff, ptr_to_trans_buff, 0);  // 0 = useless
                }
                uint64_t old_pf_address_2 = trans_buff[ptr_to_trans_buff].pf_addr;
                trans_buff[ptr_to_trans_buff].pf_addr = pf_address;
                trans_buff[ptr_to_trans_buff].ip_fold_1 = func_ip_fold_1(ip);
                trans_buff[ptr_to_trans_buff].pf_degree = degree;
                trans_buff[ptr_to_trans_buff].valid = 1;
                trans_buff[ptr_to_trans_buff].ppf_prediction = ppf_prediction;

                if (inverted_address.find(old_pf_address_2) != inverted_address.end()) {
                    for (uint32_t i = 0; i < inverted_address[old_pf_address_2].size(); ++i) {
                        if (inverted_address[old_pf_address_2][i] == ptr_to_trans_buff) {
                            inverted_address[old_pf_address_2].erase(inverted_address[old_pf_address_2].begin() + i);
                            break;
                        }
                    }
                    if (inverted_address[old_pf_address_2].size() == 0) {
                        inverted_address.erase(old_pf_address_2);
                    }
                }
                lookahead[this] = {pf_address, stride, degree - 1};
                inverted_address[pf_address].push_back(ptr_to_trans_buff);
            }
            // If we fail this cycle, we try again the next cycle
        } else {
            lookahead[this] = {};
        }
    }
}

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit,
                                         uint8_t type, uint32_t metadata_in) {
    uint64_t cl_addr = addr >> LOG2_BLOCK_SIZE;
    int64_t stride = 0;

    cache_operate_count++;
    // get boundaries of tracking set
    auto set_begin = std::next(std::begin(trackers[this]), ip % TRACKER_SETS);
    auto set_end = std::next(set_begin, TRACKER_WAYS);

    // find the current ip within the set
    auto found = std::find_if(set_begin, set_end, [ip](tracker_entry x) { return x.ip == ip; });

    // if we found a matching entry
    if (found != set_end) {
        // calculate the stride between the current address and the last address
        // no need to check for overflow since these values are downshifted
        stride = (int64_t) cl_addr - (int64_t) found->last_cl_addr;
        // Initialize prefetch state unless we somehow saw the same address twice in
        // a row or if this is the first time we've seen this stride
        if (stride != 0 && stride == found->last_stride) {
            lookahead[this] = {cl_addr, stride, PREFETCH_DEGREE, ip};
        }
    } else {
        // replace by LRU
        found = std::min_element(set_begin, set_end, [](tracker_entry x, tracker_entry y) {
            return x.last_used_cycle < y.last_used_cycle;
        });
    }

    // update tracking set
    *found = {ip, cl_addr, stride, current_cycle};
    if (cache_hit) {
        cache_hit_count++;
        if(inverted_address.find(cl_addr) != inverted_address.end()) {
            for(auto & ind: inverted_address[cl_addr]) {
                // Here ind = inverted_address[cl_addr][i]
                if(trans_buff[ind].valid == 1) {
                    retrain_ppf(trans_buff, ind, 1);        // 1 means the entry was useful
                }
            }
        }
        /*
        for (int i = 0; i < TRANSFER_BUFFER_ENTRIES; i++) {
            // Needs a break statement when the address is found
            if ((trans_buff[i].pf_addr == cl_addr) && (trans_buff[i].valid == 1)) {
                retrain_ppf(trans_buff, i, 1);        // 1 means the entry was useful
            }
        }
        */
    }
    return metadata_in;
}

uint32_t CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch,
                                      uint64_t evicted_addr, uint32_t metadata_in) {
    return metadata_in;
}

void CACHE::prefetcher_final_stats() {

    ofstream my_file;
    my_file.open(dir_name + "/prefetcher_stat_" + NAME + ".csv");
    my_file << "index, total_training, useful_training, total_prediction, true_prediction, "
               "total_prefetch, useful_prefetch, cache_operate, cache_hit" << std::endl;
    for (int i = 0; i < record_table_ind; i++) {
        my_file << i << ", "  << record_table[i].total_training
                     << ", "  << record_table[i].useful_training
                     << ", "  << record_table[i].total_prediction
                     << ", "  << record_table[i].true_prediction
                     << ", "  << record_table[i].total_prefetch
                     << ", "  << record_table[i].useful_prefetch
                     << ", "  << record_table[i].cache_operate
                     << ", "  << record_table[i].cache_hit << std::endl;
    }
    my_file << std::endl;
    my_file.close();


    my_file.open(dir_name + "/inverted_address_" + NAME + ".csv");
    my_file << "index, address, array, ip_fold_1, pf_degree, valid, ppf_prediction" << std::endl;
    int ind = 0;
    for(auto & it:inverted_address) {
        my_file << ++ind << ", " << it.first << ", ( ";
        for(auto & el: it.second) {
            my_file << el << " ";
        }
        my_file << "), ( ";

        for(auto & el: it.second) {
            my_file << trans_buff[el].ip_fold_1 << " ";
        }
        my_file << "), ( ";

        for(auto & el: it.second) {
            my_file << trans_buff[el].pf_degree << " ";
        }
        my_file << "), ( ";

        for(auto & el: it.second) {
            my_file << trans_buff[el].valid << " ";
        }
        my_file << "), ( ";

        for(auto & el: it.second) {
            my_file << trans_buff[el].ppf_prediction << " ";
        }
        my_file << ")" << std::endl;
    }
    my_file << std::endl;
    my_file.close();



    my_file.open(dir_name + "/weights_final_" + NAME + ".txt");
    my_file << "Bias: " << bias << std::endl;
    my_file << "Feature Table 1: Degree" << std::endl;
    for (int i = 0; i < FT_DEGREE_ENTRIES; i++) {
        my_file << "i: " << i << "\tw: " << ft_degree[i] << std::endl;
    }
    my_file << std::endl;
    my_file << "Feature Table 2: IP_Fold_1" << std::endl;
    for (int i = 0; i < FT_IP_FOLD_1_ENTRIES; i++) {
        my_file << "i: " << i << "\tw: " << ft_ip_fold_1[i] << std::endl;
    }
    my_file << std::endl;
    my_file << "Printing trans buff:" << std::endl;
    for (int i = 0; i < TRANSFER_BUFFER_ENTRIES; i++) {
        my_file << "Entry number: " << i << "\tAddress: " << trans_buff[i].pf_addr << "\tValid: " << trans_buff[i].valid
                << "\tDegree: " << trans_buff[i].pf_degree << std::endl;
    }
    my_file.close();
}

int ppf_decision(int degree, uint64_t ip) {
    int sum = 0;
    int feature[NUM_FEATURES_USED];
    total_prediction_count++;
    feature[0] = bias;
    int index_degree = degree;
    feature[1] = ft_degree[index_degree];
    int index_ip_fold_1 = func_ip_fold_1(ip);
    feature[2] = ft_ip_fold_1[index_ip_fold_1];
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

void retrain_ppf(transfer_buff_entry *trans_buff, int ptr_to_trans_buff, int useful) {

    int index_ip_fold_1 = trans_buff[ptr_to_trans_buff].ip_fold_1;
    int index_degree = trans_buff[ptr_to_trans_buff].pf_degree - 1;
    // index_degree is 1 less than the actual degree, e.g., for degree=1, it should index entry number 0

    total_training_count++;
    if (useful) {
        INCREMENT(ft_ip_fold_1[index_ip_fold_1]);
        INCREMENT(ft_degree[index_degree]);
        INCREMENT(bias);
        useful_training_count++;
    } else {
        DECREMENT(ft_ip_fold_1[index_ip_fold_1]);
        DECREMENT(ft_degree[index_degree]);
        DECREMENT(bias);
    }
    trans_buff[ptr_to_trans_buff].valid = 0;
}

