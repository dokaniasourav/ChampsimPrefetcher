#include <unordered_map>
#include <sys/stat.h>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <string>
#define WEIGHT_BITS 5
#define POS_MAX_WEIGHT ((1<<(WEIGHT_BITS-1))-1)
#define NEG_MAX_WEIGHT (0-(1<<(WEIGHT_BITS-1)))

/*
constexpr int INCREMENT(int &X) {
    X += 1;
    if(X == ((1<<WEIGHT_BITS) - 1)) {
        X = ((1 << WEIGHT_BITS) - 1);
    }
}
 */

int xyz;

#define INCREMENT(X) xyz = (X); xyz = (xyz == POS_MAX_WEIGHT? xyz:xyz+1); (X) = xyz;
#define DECREMENT(X) xyz = (X); xyz = (xyz == NEG_MAX_WEIGHT? xyz:xyz-1); (X) = xyz;

#define TRANSFER_BUFFER_ENTRIES 2048
#define MOVE_PTR_UP(X) ((X) = ((X) == (TRANSFER_BUFFER_ENTRIES-1) ?0:(X)+1))

#define FEATURE_IND_NAME_1 index_page_num
#define FEATURE_IND_NAME_2 index_page_off
#define FEATURE_IND_NAME_3 index_page_sig
#define FEATURE_IND_NAME_4 index_page_add
#define FEATURE_IND_NAME_5 index_pref_add
#define FEATURE_IND_NAME_6 index_inst_add

struct transfer_buff_entry {
    int valid = 0;
    int last_pred = 0;                            // indicates the prediction that was made by Perceptron

    uint32_t FEATURE_IND_NAME_1 = 0;
    uint32_t FEATURE_IND_NAME_2 = 0;
    uint32_t FEATURE_IND_NAME_3 = 0;
    uint32_t FEATURE_IND_NAME_4 = 0;
    uint32_t FEATURE_IND_NAME_5 = 0;
    uint32_t FEATURE_IND_NAME_6 = 0;
};

transfer_buff_entry trans_buff[TRANSFER_BUFFER_ENTRIES];

uint32_t t_buffer_index;

constexpr int PPF_THRESHOLD = 0;

#define NUM_FT_PAGE_NUM (1 << 10)
#define NUM_FT_PAGE_OFF (1 << 6)
#define NUM_FT_INST_SIG (1 << 10)
#define NUM_FT_PAGE_ADD (1 << 10)
#define NUM_FT_PREF_ADD (1 << 10)
#define NUM_FT_INST_ADD (1 << 10)

#define SIZE_OF_ARRAY(arr_name) (sizeof(arr_name) / sizeof((arr_name)[0]))
#define INDEX_TO(arr_name, ind_value) arr_name[((ind_value) & (SIZE_OF_ARRAY(arr_name) - 1))]

#define FEATURE_VAR_NAME_1 ft_page_num
#define FEATURE_VAR_NAME_2 ft_page_off
#define FEATURE_VAR_NAME_3 ft_page_sig
#define FEATURE_VAR_NAME_4 ft_page_add
#define FEATURE_VAR_NAME_5 ft_pref_add
#define FEATURE_VAR_NAME_6 ft_inst_add

int ft_page_bias;
int FEATURE_VAR_NAME_1[NUM_FT_PAGE_NUM];
int FEATURE_VAR_NAME_2[NUM_FT_PAGE_OFF];
int FEATURE_VAR_NAME_3[NUM_FT_INST_SIG];
int FEATURE_VAR_NAME_4[NUM_FT_PAGE_ADD];
int FEATURE_VAR_NAME_5[NUM_FT_PREF_ADD];
int FEATURE_VAR_NAME_6[NUM_FT_INST_ADD];

#define DEF_AS_STR_(VAR) #VAR
#define DEF_AS_STR(VAR) DEF_AS_STR_(VAR)

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

    if (useful) {
        INCREMENT(INDEX_TO(FEATURE_VAR_NAME_1, trans_buff[index].FEATURE_IND_NAME_1 ));
        INCREMENT(INDEX_TO(FEATURE_VAR_NAME_2, trans_buff[index].FEATURE_IND_NAME_2 ));
        INCREMENT(INDEX_TO(FEATURE_VAR_NAME_3, trans_buff[index].FEATURE_IND_NAME_3 ));
        INCREMENT(INDEX_TO(FEATURE_VAR_NAME_4, trans_buff[index].FEATURE_IND_NAME_4 ));
        INCREMENT(INDEX_TO(FEATURE_VAR_NAME_5, trans_buff[index].FEATURE_IND_NAME_5 ));
        INCREMENT(INDEX_TO(FEATURE_VAR_NAME_6, trans_buff[index].FEATURE_IND_NAME_6 ));
    } else {
        DECREMENT(INDEX_TO(FEATURE_VAR_NAME_1, trans_buff[index].FEATURE_IND_NAME_1 ));
        DECREMENT(INDEX_TO(FEATURE_VAR_NAME_2, trans_buff[index].FEATURE_IND_NAME_2 ));
        DECREMENT(INDEX_TO(FEATURE_VAR_NAME_3, trans_buff[index].FEATURE_IND_NAME_3 ));
        DECREMENT(INDEX_TO(FEATURE_VAR_NAME_4, trans_buff[index].FEATURE_IND_NAME_4 ));
        DECREMENT(INDEX_TO(FEATURE_VAR_NAME_5, trans_buff[index].FEATURE_IND_NAME_5 ));
        DECREMENT(INDEX_TO(FEATURE_VAR_NAME_6, trans_buff[index].FEATURE_IND_NAME_6 ));
    }
    trans_buff[index].valid = 0;
    total_training_count++;
    if(useful) {
        useful_training_count++;
    }
}

/*
 * Returns the sum of weight index array
 * */
int ppf_decision(transfer_buff_entry entry_values) {

    int feature_sum = ft_page_bias;
    feature_sum += INDEX_TO(FEATURE_VAR_NAME_1, entry_values.FEATURE_IND_NAME_1);
    feature_sum += INDEX_TO(FEATURE_VAR_NAME_2, entry_values.FEATURE_IND_NAME_2);
    feature_sum += INDEX_TO(FEATURE_VAR_NAME_3, entry_values.FEATURE_IND_NAME_3);
    feature_sum += INDEX_TO(FEATURE_VAR_NAME_4, entry_values.FEATURE_IND_NAME_4);
    feature_sum += INDEX_TO(FEATURE_VAR_NAME_5, entry_values.FEATURE_IND_NAME_5);
    feature_sum += INDEX_TO(FEATURE_VAR_NAME_6, entry_values.FEATURE_IND_NAME_6);

    total_prediction_count++;
    return feature_sum;
}

