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

#define TRANSFER_BUFFER_ENTRIES 2048
#define MOVE_PTR_UP(X) (X = (X == (TRANSFER_BUFFER_ENTRIES-1) ?0:X+1))

struct transfer_buff_entry {
    int valid = 0;
    int last_pred = 0;                            // indicates the prediction that was made by Perceptron

    uint32_t ind_page_number = 0;
    uint32_t ind_page_offset = 0;
    uint32_t ind_page_address = 0;
    uint32_t ind_page_signature = 0;
    uint32_t ind_prefetch_add = 0;
};

transfer_buff_entry trans_buff[TRANSFER_BUFFER_ENTRIES];

uint32_t t_buffer_index;

constexpr int PPF_THRESHOLD = 0;

#define NUM_FT_PAGE_ADD (1 << 10)
#define NUM_FT_PREF_ADD (1 << 10)

int ft_page_bias;
int ft_page_add[NUM_FT_PAGE_ADD];
int ft_pref_add[NUM_FT_PREF_ADD];
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

void retrain_ppf(uint32_t index, int useful);
int ppf_decision(transfer_buff_entry entry_values);

void retrain_ppf(uint32_t index, int useful) {
    uint32_t index_pg_no = trans_buff[index].ind_page_number     & (ST_SET * ST_WAY - 1);
    uint32_t index_pg_of = trans_buff[index].ind_page_offset     & (ST_SET * ST_WAY - 1);
    uint32_t index_pg_si = trans_buff[index].ind_page_signature  & (ST_SET * ST_WAY - 1);
    uint32_t index_pg_ad = trans_buff[index].ind_page_address    & (NUM_FT_PAGE_ADD - 1);
    uint32_t index_pf_ad = trans_buff[index].ind_prefetch_add    & (NUM_FT_PREF_ADD - 1);

    total_training_count++;
    if(useful) {
        useful_training_count++;
    }

    if (useful) {
        INCREMENT(ft_page_num[index_pg_no]);
        INCREMENT(ft_page_off[index_pg_of]);
        INCREMENT(ft_page_sig[index_pg_si]);
        INCREMENT(ft_page_add[index_pg_ad]);
        INCREMENT(ft_pref_add[index_pf_ad]);
        INCREMENT(ft_page_bias);
    } else {
        DECREMENT(ft_page_num[index_pg_no]);
        DECREMENT(ft_page_off[index_pg_of]);
        DECREMENT(ft_page_sig[index_pg_si]);
        DECREMENT(ft_page_add[index_pg_ad]);
        DECREMENT(ft_pref_add[index_pf_ad]);
        DECREMENT(ft_page_bias);
    }
    trans_buff[index].valid = 0;
}

/*
 * Returns the sum of weight index array
 * */
int ppf_decision(transfer_buff_entry entry_values) {
    uint32_t index_pg_no = entry_values.ind_page_number    & (ST_SET * ST_WAY - 1);
    uint32_t index_pg_of = entry_values.ind_page_offset    & (ST_SET * ST_WAY - 1);
    uint32_t index_pg_si = entry_values.ind_page_signature & (ST_SET * ST_WAY - 1);
    uint32_t index_pg_ad = entry_values.ind_page_address   & (NUM_FT_PAGE_ADD - 1);
    uint32_t index_pf_ad = entry_values.ind_prefetch_add   & (NUM_FT_PREF_ADD - 1);

    int feature_sum = ft_page_bias;
    feature_sum += ft_page_num[index_pg_no];
    feature_sum += ft_page_off[index_pg_of];
    feature_sum += ft_page_sig[index_pg_si];
    feature_sum += ft_page_add[index_pg_ad];
    feature_sum += ft_pref_add[index_pf_ad];

    total_prediction_count++;
    return feature_sum;
}

