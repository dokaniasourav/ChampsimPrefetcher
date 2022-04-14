#include <unordered_map>
#include <sys/stat.h>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <string>
#define WEIGHT_BITS 5

/*
constexpr int INCREMENT(int &X) {
    X += 1;
    if(X == ((1<<WEIGHT_BITS) - 1)) {
        X = ((1 << WEIGHT_BITS) - 1);
    }
}
 */

#define INCREMENT(X) (X = (X == ((1<<(WEIGHT_BITS-1))-1)) ?X:X+1)
#define DECREMENT(X) (X = (X == (0-(1<<(WEIGHT_BITS-1)))) ?X:X-1)
#define THRESHOLD(X) (X = (X > 0) ?1:0)

#define TRANSFER_BUFFER_ENTRIES 1024
#define MOVE_PTR_UP(X) (X = (X == (TRANSFER_BUFFER_ENTRIES-1) ?0:X+1))

struct transfer_buff_entry {
    int valid = 0;
    int last_pred = 0;                            // indicates the prediction that was made by Perceptron
    uint64_t pf_address;

    uint32_t page_number = 0;
    uint32_t page_offset = 0;
    uint32_t page_address = 0;
    uint32_t page_signature = 0;
};

transfer_buff_entry trans_buff[TRANSFER_BUFFER_ENTRIES];

int transfer_buffer_index;

constexpr int PPF_THRESHOLD = 0;

constexpr int FT_PAGE_BIT_LOC = 10;
constexpr int FT_PAGES_TOTAL = (1 << FT_PAGE_BIT_LOC);

int ft_page_bias;
int ft_page_add[FT_PAGES_TOTAL];
int ft_page_num[ST_SET * ST_WAY];
int ft_page_off[ST_SET * ST_WAY];
int ft_page_sig[ST_SET * ST_WAY];


/*********************************************************************************************************************/
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
    uint64_t cycle_operate;
};

std::string dir_name;

int record_table_ind = 0;
uint64_t next_cycle_update = 0;
recorder_str record_table[REC_TB_SIZE];
constexpr int CYCLE_UPDATE_INTERVAL = 1000000;

uint64_t cache_operate_count = 0;
uint64_t cycle_operate_count = 0;
uint64_t cache_hit_count = 0;

uint64_t total_training_count = 0;
uint64_t useful_training_count = 0;
uint64_t total_prediction_count = 0;
uint64_t true_prediction_count = 0;
/*********************************************************************************************************************/

void retrain_ppf(int trans_buffer_index, int useful);
int ppf_decision(transfer_buff_entry entry_values);

void retrain_ppf(int ptr_to_trans_buff, int useful) {
    int index_pn = trans_buff[ptr_to_trans_buff].page_number    & (ST_SET*ST_WAY - 1);
    int index_po = trans_buff[ptr_to_trans_buff].page_offset    & (ST_SET*ST_WAY - 1);
    int index_ps = trans_buff[ptr_to_trans_buff].page_signature & (ST_SET*ST_WAY - 1);
    int index_pa = trans_buff[ptr_to_trans_buff].page_address   & (FT_PAGES_TOTAL - 1);

    total_training_count++;
    if(useful) {
        useful_training_count++;
    }

    if (useful) {
        INCREMENT(ft_page_num[index_pn]);
        INCREMENT(ft_page_off[index_po]);
        INCREMENT(ft_page_sig[index_ps]);
        INCREMENT(ft_page_add[index_pa]);
        INCREMENT(ft_page_bias);
    } else {
        DECREMENT(ft_page_num[index_pn]);
        DECREMENT(ft_page_off[index_po]);
        DECREMENT(ft_page_sig[index_ps]);
        DECREMENT(ft_page_add[index_pa]);
        DECREMENT(ft_page_bias);
    }
    trans_buff[ptr_to_trans_buff].valid = 0;
}

/*
 * Returns the sum of weight index array
 * */
int ppf_decision(transfer_buff_entry entry_values) {

    int index_pn = entry_values.page_number    & (ST_SET*ST_WAY - 1);
    int index_po = entry_values.page_offset    & (ST_SET*ST_WAY - 1);
    int index_ps = entry_values.page_signature & (ST_SET*ST_WAY - 1);
    int index_pa = entry_values.page_address   & (FT_PAGES_TOTAL - 1);

    int feature_sum = ft_page_bias;
    feature_sum += ft_page_num[index_pn];
    feature_sum += ft_page_off[index_po];
    feature_sum += ft_page_sig[index_ps];
    feature_sum += ft_page_add[index_pa];

    total_prediction_count++;

    return feature_sum;
}

