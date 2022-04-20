#include <unordered_map>
#include <sys/stat.h>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <string>
#define WEIGHT_BITS 6
#define POS_MAX_WEIGHT ((1<<(WEIGHT_BITS-1))-1)
#define NEG_MAX_WEIGHT (0-(1<<(WEIGHT_BITS-1)))

/**
* constexpr int INCREMENT(int &X) {
*     X += 1;
*     if(X == ((1<<WEIGHT_BITS) - 1)) {
*         X = ((1 << WEIGHT_BITS) - 1);
*     }
* }
**/

int xyz;

#define INCREMENT(X) xyz = (X); xyz = (xyz == POS_MAX_WEIGHT? (xyz-4):(xyz+1)); (X) = xyz;
#define DECREMENT(X) xyz = (X); xyz = (xyz == NEG_MAX_WEIGHT? (xyz+4):(xyz-1)); (X) = xyz;

#define TRANSFER_BUFFER_ENTRIES 4096
#define MOVE_PTR_UP(X) ((X) = ((X) == (TRANSFER_BUFFER_ENTRIES-1) ?0:(X)+1))

#define FEATURE_IND_NAME_1 index_page_num
#define FEATURE_IND_NAME_2 index_page_off
#define FEATURE_IND_NAME_3 index_page_sig
#define FEATURE_IND_NAME_4 index_page_add
#define FEATURE_IND_NAME_5 index_pref_add
#define FEATURE_IND_NAME_6 index_inst_add
#define FEATURE_IND_NAME_7 index_misc_add
#define FEATURE_IND_NAME_8 index_misc_val

#define FEATURE_PAD_VAL_1 10
#define FEATURE_PAD_VAL_2 0
#define FEATURE_PAD_VAL_3 0
#define FEATURE_PAD_VAL_4 6
#define FEATURE_PAD_VAL_5 6
#define FEATURE_PAD_VAL_6 6
#define FEATURE_PAD_VAL_7 0
#define FEATURE_PAD_VAL_8 0


struct transfer_buff_entry {
    int valid = 0;
    int last_pred = 0;                            // indicates the prediction that was made by Perceptron

    uint64_t FEATURE_IND_NAME_1 = 0;
    uint64_t FEATURE_IND_NAME_2 = 0;
    uint64_t FEATURE_IND_NAME_3 = 0;
    uint64_t FEATURE_IND_NAME_4 = 0;
    uint64_t FEATURE_IND_NAME_5 = 0;
    uint64_t FEATURE_IND_NAME_6 = 0;
    uint64_t FEATURE_IND_NAME_7 = 0;
    uint64_t FEATURE_IND_NAME_8 = 0;
};

transfer_buff_entry trans_buff[TRANSFER_BUFFER_ENTRIES];

uint32_t t_buffer_index;

constexpr int PPF_THRESHOLD = 0;

#define NUM_FT_PAGE_NUM (1 << 10)
#define NUM_FT_PAGE_OFF (1 << 6)
#define NUM_FT_INST_SIG (1 << 11)
#define NUM_FT_PAGE_ADD (1 << 10)
#define NUM_FT_PREF_ADD (1 << 11)
#define NUM_FT_INST_ADD (1 << 11)
#define NUM_FT_MISC_ADD (1 << 10)
#define NUM_FT_MISC_VAL (1 << 10)

#define SIZE_OF_ARRAY(arr_name) (sizeof(arr_name) / sizeof((arr_name)[0]))
#define INDEX_TO(arr_name, ind_value) arr_name[((ind_value) & (SIZE_OF_ARRAY(arr_name) - 1))]

#define FEATURE_VAR_NAME_1 ft_page_num
#define FEATURE_VAR_NAME_2 ft_page_off
#define FEATURE_VAR_NAME_3 ft_page_sig
#define FEATURE_VAR_NAME_4 ft_page_add
#define FEATURE_VAR_NAME_5 ft_pref_add
#define FEATURE_VAR_NAME_6 ft_inst_add
#define FEATURE_VAR_NAME_7 ft_misc_add
#define FEATURE_VAR_NAME_8 ft_misc_val

#define FEATURE_DEBUG_NAME_1 ft_page_num_debug
#define FEATURE_DEBUG_NAME_2 ft_page_off_debug
#define FEATURE_DEBUG_NAME_3 ft_page_sig_debug
#define FEATURE_DEBUG_NAME_4 ft_page_add_debug
#define FEATURE_DEBUG_NAME_5 ft_pref_add_debug
#define FEATURE_DEBUG_NAME_6 ft_inst_add_debug
#define FEATURE_DEBUG_NAME_7 ft_misc_add_debug
#define FEATURE_DEBUG_NAME_8 ft_misc_val_debug

uint64_t FEATURE_DEBUG_NAME_1[NUM_FT_PAGE_NUM];
uint64_t FEATURE_DEBUG_NAME_2[NUM_FT_PAGE_OFF];
uint64_t FEATURE_DEBUG_NAME_3[NUM_FT_INST_SIG];
uint64_t FEATURE_DEBUG_NAME_4[NUM_FT_PAGE_ADD];
uint64_t FEATURE_DEBUG_NAME_5[NUM_FT_PREF_ADD];
uint64_t FEATURE_DEBUG_NAME_6[NUM_FT_INST_ADD];
uint64_t FEATURE_DEBUG_NAME_7[NUM_FT_MISC_ADD];
uint64_t FEATURE_DEBUG_NAME_8[NUM_FT_MISC_VAL];

int ft_page_bias;
int FEATURE_VAR_NAME_1[NUM_FT_PAGE_NUM];
int FEATURE_VAR_NAME_2[NUM_FT_PAGE_OFF];
int FEATURE_VAR_NAME_3[NUM_FT_INST_SIG];
int FEATURE_VAR_NAME_4[NUM_FT_PAGE_ADD];
int FEATURE_VAR_NAME_5[NUM_FT_PREF_ADD];
int FEATURE_VAR_NAME_6[NUM_FT_INST_ADD];
int FEATURE_VAR_NAME_7[NUM_FT_MISC_ADD];
int FEATURE_VAR_NAME_8[NUM_FT_MISC_VAL];

#define DEF_AS_STR_(VAR) #VAR
#define DEF_AS_STR(VAR) DEF_AS_STR_(VAR)

/*********************************************************************************************************************/
#define REC_TB_SIZE 2048

#define RECORDER_VAR_NAME_01 total_training
#define RECORDER_VAR_NAME_02 useful_training
#define RECORDER_VAR_NAME_03 total_prediction
#define RECORDER_VAR_NAME_04 true_prediction
#define RECORDER_VAR_NAME_05 total_prefetch
#define RECORDER_VAR_NAME_06 useful_prefetch
#define RECORDER_VAR_NAME_07 cache_operate
#define RECORDER_VAR_NAME_08 cache_hit
#define RECORDER_VAR_NAME_09 cycle_operate
#define RECORDER_VAR_NAME_10 success_pref
#define RECORDER_VAR_NAME_11 prefetch_accuracy
#define RECORDER_VAR_NAME_12 extra_var_1


struct recorder_str {
    uint64_t RECORDER_VAR_NAME_01;
    uint64_t RECORDER_VAR_NAME_02;
    uint64_t RECORDER_VAR_NAME_03;
    uint64_t RECORDER_VAR_NAME_04;
    uint64_t RECORDER_VAR_NAME_05;
    uint64_t RECORDER_VAR_NAME_06;
    uint64_t RECORDER_VAR_NAME_07;
    uint64_t RECORDER_VAR_NAME_08;
    uint64_t RECORDER_VAR_NAME_09;
    uint64_t RECORDER_VAR_NAME_10;
    uint64_t RECORDER_VAR_NAME_11;
    uint64_t RECORDER_VAR_NAME_12;
};

std::string dir_name;
int record_table_ind = 0;
uint64_t next_cycle_update = 0;
recorder_str record_table[REC_TB_SIZE];
constexpr int CYCLE_UPDATE_INTERVAL = 100000;

/*********************************************************************************************************************/

void retrain_ppf(uint32_t index, int useful);
int ppf_decision(transfer_buff_entry entry_values);

void retrain_ppf(uint32_t index, int useful) {

    uint32_t index_var_1 = (trans_buff[index].FEATURE_IND_NAME_1 >> FEATURE_PAD_VAL_1);
    uint32_t index_var_2 = (trans_buff[index].FEATURE_IND_NAME_2 >> FEATURE_PAD_VAL_2);
    uint32_t index_var_3 = (trans_buff[index].FEATURE_IND_NAME_3 >> FEATURE_PAD_VAL_3);
    uint32_t index_var_4 = (trans_buff[index].FEATURE_IND_NAME_4 >> FEATURE_PAD_VAL_4);
    uint32_t index_var_5 = (trans_buff[index].FEATURE_IND_NAME_5 >> FEATURE_PAD_VAL_5);
    uint32_t index_var_6 = (trans_buff[index].FEATURE_IND_NAME_6 >> FEATURE_PAD_VAL_6);
    uint32_t index_var_7 = (trans_buff[index].FEATURE_IND_NAME_7 >> FEATURE_PAD_VAL_7);
    uint32_t index_var_8 = (trans_buff[index].FEATURE_IND_NAME_8 >> FEATURE_PAD_VAL_8);

    if (useful) {
        INCREMENT(INDEX_TO(FEATURE_VAR_NAME_1, index_var_1 ));
        INCREMENT(INDEX_TO(FEATURE_VAR_NAME_2, index_var_2 ));
        INCREMENT(INDEX_TO(FEATURE_VAR_NAME_3, index_var_3 ));
        INCREMENT(INDEX_TO(FEATURE_VAR_NAME_4, index_var_4 ));
        INCREMENT(INDEX_TO(FEATURE_VAR_NAME_5, index_var_5 ));
        INCREMENT(INDEX_TO(FEATURE_VAR_NAME_6, index_var_6 ));
        INCREMENT(INDEX_TO(FEATURE_VAR_NAME_7, index_var_7 ));
        INCREMENT(INDEX_TO(FEATURE_VAR_NAME_8, index_var_8 ));
    } else {
        DECREMENT(INDEX_TO(FEATURE_VAR_NAME_1, index_var_1 ));
        DECREMENT(INDEX_TO(FEATURE_VAR_NAME_2, index_var_2 ));
        DECREMENT(INDEX_TO(FEATURE_VAR_NAME_3, index_var_3 ));
        DECREMENT(INDEX_TO(FEATURE_VAR_NAME_4, index_var_4 ));
        DECREMENT(INDEX_TO(FEATURE_VAR_NAME_5, index_var_5 ));
        DECREMENT(INDEX_TO(FEATURE_VAR_NAME_6, index_var_6 ));
        DECREMENT(INDEX_TO(FEATURE_VAR_NAME_7, index_var_7 ));
        DECREMENT(INDEX_TO(FEATURE_VAR_NAME_8, index_var_8 ));
    }
    trans_buff[index].valid = 0;
    record_table[record_table_ind].total_training++;
    if(useful) {
        record_table[record_table_ind].useful_training++;
    }
}

/*
 * Returns the sum of weight index array
 * */
int ppf_decision(transfer_buff_entry entry_values) {

    uint32_t index_var_1 = (entry_values.FEATURE_IND_NAME_1 >> FEATURE_PAD_VAL_1);
    uint32_t index_var_2 = (entry_values.FEATURE_IND_NAME_2 >> FEATURE_PAD_VAL_2);
    uint32_t index_var_3 = (entry_values.FEATURE_IND_NAME_3 >> FEATURE_PAD_VAL_3);
    uint32_t index_var_4 = (entry_values.FEATURE_IND_NAME_4 >> FEATURE_PAD_VAL_4);
    uint32_t index_var_5 = (entry_values.FEATURE_IND_NAME_5 >> FEATURE_PAD_VAL_5);
    uint32_t index_var_6 = (entry_values.FEATURE_IND_NAME_6 >> FEATURE_PAD_VAL_6);
    uint32_t index_var_7 = (entry_values.FEATURE_IND_NAME_7 >> FEATURE_PAD_VAL_7);
    uint32_t index_var_8 = (entry_values.FEATURE_IND_NAME_8 >> FEATURE_PAD_VAL_8);

    int feature_sum = ft_page_bias;
    feature_sum += INDEX_TO(FEATURE_VAR_NAME_1, index_var_1);
    feature_sum += INDEX_TO(FEATURE_VAR_NAME_2, index_var_2);
    feature_sum += INDEX_TO(FEATURE_VAR_NAME_3, index_var_3);
    feature_sum += INDEX_TO(FEATURE_VAR_NAME_4, index_var_4);
    feature_sum += INDEX_TO(FEATURE_VAR_NAME_5, index_var_5);
    feature_sum += INDEX_TO(FEATURE_VAR_NAME_6, index_var_6);
    feature_sum += INDEX_TO(FEATURE_VAR_NAME_7, index_var_7);
    feature_sum += INDEX_TO(FEATURE_VAR_NAME_8, index_var_8);

    record_table[record_table_ind].total_prediction++;
    return feature_sum;
}

