#define TRANSFER_BUFFER_ENTRIES 1024

struct transfer_buff_entry {
    uint64_t pf_addr = 0;
    int ip_fold_1 = 0;
    int pf_degree = 0;
    int valid = 0;
    int ppf_prediction = 0;                            // indicates the prediction that was made by Perceptron
};

transfer_buff_entry trans_buff[TRANSFER_BUFFER_ENTRIES];

int ppf_decision(int degree, uint64_t ip);

