#include <iostream>
#include <stdint.h>
#include "ppf_spp_dev.h"
#include "ppf_setup.h"
#include "cache.h"

SIGNATURE_TABLE ST;
PATTERN_TABLE PT;
PREFETCH_FILTER FILTER;
GLOBAL_REGISTER GHR;

void CACHE::prefetcher_initialize() {
    std::cout << NAME << "SPP DEV Prefetcher"
    std::endl;
}

void CACHE::prefetcher_cycle_operate() {}

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip,
                                         uint8_t cache_hit, uint8_t type, uint32_t metadata_in) {
    uint64_t page = addr >> LOG2_PAGE_SIZE;
    uint32_t page_offset = (addr >> LOG2_BLOCK_SIZE) & (PAGE_SIZE / BLOCK_SIZE - 1);
    uint32_t last_sig = 0, curr_sig = 0, confidence_q[MSHR_SIZE], depth = 0;
    int32_t delta = 0, delta_q[MSHR_SIZE];

    for (uint32_t i = 0; i < MSHR_SIZE; i++) {
        confidence_q[i] = 0;
        delta_q[i] = 0;
    }
    confidence_q[0] = 100;      // Level zero confidence 100

    // Percentage of useful prefetch
    GHR.global_accuracy = GHR.pf_issued ? ((100 * GHR.pf_useful) / GHR.pf_issued) : 0;

    SPP_DP( std::cout << std::endl << "[ChampSim] " << __func__ << " addr: "
            << hex << addr << " cache_line: " << (addr >> LOG2_BLOCK_SIZE);
            std::cout << " page: " << page << " page_offset: " << dec << page_offset
            << std::endl;);

    /*
    // Stage 1: Read and update a sig stored in ST
    // last_sig and delta are used to update (sig, delta) correlation in PT
    // curr_sig is used to read prefetch candidates in PT

    // Also check the prefetch filter in parallel to update global accuracy
    // counters
     */

    ST.read_and_update_sig(page, page_offset, last_sig, curr_sig, delta);

    FILTER.check(addr, L2C_DEMAND);

    // Stage 2: Update delta patterns stored in PT
    if (last_sig)
        PT.update_pattern(last_sig, delta);

    // Stage 3: Start prefetching
    uint64_t base_addr = addr;
    uint32_t lookahead_conf = 100, pf_q_head = 0, pf_q_tail = 0;
    uint8_t do_lookahead = 0;

#ifdef LOOKAHEAD_ON
    do {
#endif
        uint32_t lookahead_way = PT_WAY;
        PT.read_pattern(curr_sig, delta_q, confidence_q, lookahead_way, lookahead_conf, pf_q_tail, depth);

        do_lookahead = 0;
        for (uint32_t i = pf_q_head; i < pf_q_tail; i++) {
            if (confidence_q[i] >= PF_THRESHOLD) {
                uint64_t pf_addr = (base_addr & ~(BLOCK_SIZE - 1)) + (delta_q[i] << LOG2_BLOCK_SIZE);

                if ((addr & ~(PAGE_SIZE - 1)) == (pf_addr & ~(PAGE_SIZE - 1))) {
                    // Prefetch request is in the same physical page

                    transfer_buff_entry entry_values;
                    entry_values.page_number = page;
                    entry_values.page_offset = page_offset;
                    entry_values.page_signature = curr_sig;
                    entry_values.page_address = addr;

                    int ppf_value = ppf_decision(entry_values);
                    if(ppf_value > PPF_THRESHOLD) {
                        if (FILTER.check(pf_addr,
                                         ((confidence_q[i] >= FILL_THRESHOLD) ? SPP_L2C_PREFETCH : SPP_LLC_PREFETCH))) {
                            int success = prefetch_line(pf_addr, (confidence_q[i] >= FILL_THRESHOLD), 0);
                            // Use addr (not base_addr) to obey the same
                            // physical page boundary
                            if(success == 1) {
                                if (confidence_q[i] >= FILL_THRESHOLD) {
                                    GHR.pf_issued++;
                                    if (GHR.pf_issued > GLOBAL_COUNTER_MAX) {
                                        GHR.pf_issued >>= 1;
                                        GHR.pf_useful >>= 1;
                                    }
                                    /*
                                    SPP_DP(std::cout << "[ChampSim] SPP L2 prefetch issued GHR.pf_issued: " << GHR.pf_issued
                                                     << " GHR.pf_useful: " << GHR.pf_useful << std::endl;);
                                    */
                                }
                                MOVE_PTR_UP(transfer_buffer_index);

                                if(trans_buff[transfer_buffer_index].valid == 1) {
                                    retrain_ppf(transfer_buffer_index, 0);
                                }
                                entry_values.valid = 1;
                                entry_values.last_pred = ppf_value;
                                trans_buff[transfer_buffer_index] = entry_values;
                                /*
                                SPP_DP(std::cout << "[ChampSim] " << __func__ << " base_addr: " << hex << base_addr
                                                 << " pf_addr: "
                                                 << pf_addr;
                                               std::cout << " pf_cache_line: " << (pf_addr >> LOG2_BLOCK_SIZE);
                                               std::cout << " prefetch_delta: " << dec << delta_q[i] << " confidence: "
                                                         << confidence_q[i];
                                               std::cout << " depth: " << i << " fill_level: "
                                                         << ((confidence_q[i] >= FILL_THRESHOLD) ? FILL_L2 : FILL_LLC)
                                                         << std::endl;);
                                                         */
                            }
                        }
                    } else {
                        // Do not issue any prefetch..  no need to even check
                    }
                } else {
                    // Prefetch request is crossing the physical page boundary
#ifdef GHR_ON
                    // Store this prefetch request in GHR to bootstrap SPP learning when
                    // we see a ST miss (i.e., accessing a new page)
                    GHR.update_entry(curr_sig, confidence_q[i], (pf_addr >> LOG2_BLOCK_SIZE) & 0x3F, delta_q[i]);
#endif
                }
                do_lookahead = 1;
                pf_q_head++;
            }
        }

        // Update base_addr and curr_sig
        if (lookahead_way < PT_WAY) {
            uint32_t set = get_hash(curr_sig) % PT_SET;
            base_addr += (PT.delta[set][lookahead_way] << LOG2_BLOCK_SIZE);

            // PT.delta uses a 7-bit sign magnitude representation to generate
            // sig_delta
            // int sig_delta = (PT.delta[set][lookahead_way] < 0) ? ((((-1) *
            // PT.delta[set][lookahead_way]) & 0x3F) + 0x40) :
            // PT.delta[set][lookahead_way];
            int sig_delta = (PT.delta[set][lookahead_way] < 0) ? (((-1) * PT.delta[set][lookahead_way]) +
                                                                  (1 << (SIG_DELTA_BIT - 1)))
                                                               : PT.delta[set][lookahead_way];
            curr_sig = ((curr_sig << SIG_SHIFT) ^ sig_delta) & SIG_MASK;
        }

        SPP_DP(std::cout << "Looping curr_sig: " << hex << curr_sig << " base_addr: " << base_addr << dec;
                       std::cout << " pf_q_head: " << pf_q_head << " pf_q_tail: " << pf_q_tail << " depth: " << depth
                                 << std::endl;);
#ifdef LOOKAHEAD_ON
    } while (do_lookahead);
#endif

    return metadata_in;
}

uint32_t
CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t match, uint8_t prefetch, uint64_t evicted_addr,
                             uint32_t metadata_in) {
#ifdef FILTER_ON
    SPP_DP(std::cout << std::endl;);
    FILTER.check(evicted_addr, L2C_EVICT);
#endif

    return metadata_in;
}

void CACHE::prefetcher_final_stats() {}

// TODO_: Find a good 64-bit hash function
uint64_t get_hash(uint64_t key) {
    // Robert Jenkins' 32 bit mix function
    key += (key << 12);
    key ^= (key >> 22);
    key += (key << 4);
    key ^= (key >> 9);
    key += (key << 10);
    key ^= (key >> 2);
    key += (key << 7);
    key ^= (key >> 12);

    // Knuth's multiplicative method
    key = (key >> 3) * 2654435761;

    return key;
}

void SIGNATURE_TABLE::read_and_update_sig(uint64_t page, uint32_t page_offset, uint32_t &last_sig, uint32_t &curr_sig,
                                          int32_t &delta) {
    uint32_t set = get_hash(page) % ST_SET;
    uint32_t match = ST_WAY;
    uint32_t partial_page = page & ST_TAG_MASK;
    uint8_t ST_hit = 0;
    int sig_delta = 0;

    /*
    SPP_DP(std::cout << "[ST] " << __func__ << " page: " << hex << page << " partial_page: " << partial_page << dec
                     << std::endl;);
    */

    // Case 1: Hit
    for (match = 0; match < ST_WAY; match++) {
        if (valid[set][match] && (tag[set][match] == partial_page)) {
            last_sig = sig[set][match];
            delta = page_offset - last_offset[set][match];

            if (delta) {
                // Build a new sig based on 7-bit sign magnitude representation of delta
                // sig_delta = (delta < 0) ? ((((-1) * delta) & 0x3F) + 0x40) : delta;
                sig_delta = (delta < 0) ? (((-1) * delta) + (1 << (SIG_DELTA_BIT - 1))) : delta;
                sig[set][match] = ((last_sig << SIG_SHIFT) ^ sig_delta) & SIG_MASK;
                curr_sig = sig[set][match];
                last_offset[set][match] = page_offset;

                SPP_DP(std::cout << "[ST] " << __func__ << " hit set: " << set << " way: " << match;
                               std::cout << " valid: " << valid[set][match] << " tag: " << hex << tag[set][match];
                               std::cout
                                       << " last_sig: " << last_sig << " curr_sig: " << curr_sig;
                               std::cout << " delta: " << dec << delta << " last_offset: " << page_offset
                                         << std::endl;);
            } else
                last_sig = 0; // Hitting the same cache line, delta is zero

            ST_hit = 1;
            break;
        }
    }

    // Case 2: Invalid
    if (match == ST_WAY) {
        for (match = 0; match < ST_WAY; match++) {
            if (valid[set][match] == 0) {
                valid[set][match] = 1;
                tag[set][match] = partial_page;
                sig[set][match] = 0;
                curr_sig = sig[set][match];
                last_offset[set][match] = page_offset;

                SPP_DP(std::cout << "[ST] " << __func__ << " invalid set: " << set << " way: " << match;
                               std::cout << " valid: " << valid[set][match] << " tag: " << hex << partial_page;
                               std::cout << " sig: " << sig[set][match] << " last_offset: " << dec << page_offset
                                         << std::endl;);

                break;
            }
        }
    }

    // Case 3: Miss
    if (match == ST_WAY) {
        for (match = 0; match < ST_WAY; match++) {
            if (lru[set][match] == ST_WAY - 1) { // Find replacement victim
                tag[set][match] = partial_page;
                sig[set][match] = 0;
                curr_sig = sig[set][match];
                last_offset[set][match] = page_offset;

                SPP_DP(std::cout << "[ST] " << __func__ << " miss set: " << set << " way: " << match;
                               std::cout << " valid: " << valid[set][match] << " victim tag: " << hex << tag[set][match]
                                         << " new tag: " << partial_page;
                               std::cout << " sig: " << sig[set][match] << " last_offset: " << dec << page_offset
                                         << std::endl;);

                break;
            }
        }

#ifdef SPP_SANITY_CHECK
        // Assertion
        if (match == ST_WAY) {
            std::cout << "[ST] Cannot find a replacement victim!" << std::endl;
            assert(0);
        }
#endif
    }

#ifdef GHR_ON
    if (ST_hit == 0) {
        uint32_t GHR_found = GHR.check_entry(page_offset);
        if (GHR_found < MAX_GHR_ENTRY) {
            sig_delta = (GHR.delta[GHR_found] < 0) ? (((-1) * GHR.delta[GHR_found]) + (1 << (SIG_DELTA_BIT - 1)))
                                                   : GHR.delta[GHR_found];
            sig[set][match] = ((GHR.sig[GHR_found] << SIG_SHIFT) ^ sig_delta) & SIG_MASK;
            curr_sig = sig[set][match];
        }
    }
#endif

    // Update LRU
    for (uint32_t way = 0; way < ST_WAY; way++) {
        if (lru[set][way] < lru[set][match]) {
            lru[set][way]++;

#ifdef SPP_SANITY_CHECK
            // Assertion
            if (lru[set][way] >= ST_WAY) {
                std::cout << "[ST] LRU value is wrong! set: " << set << " way: " << way << " lru: " << lru[set][way]
                          << std::endl;
                assert(0);
            }
#endif
        }
    }
    lru[set][match] = 0; // Promote to the MRU position
}

void PATTERN_TABLE::update_pattern(uint32_t last_sig, int curr_delta) {
    // Update (sig, delta) correlation
    uint32_t set = get_hash(last_sig) % PT_SET, match = 0;

    // Case 1: Hit
    for (match = 0; match < PT_WAY; match++) {
        if (delta[set][match] == curr_delta) {
            c_delta[set][match]++;
            c_sig[set]++;
            if (c_sig[set] > C_SIG_MAX) {
                for (uint32_t way = 0; way < PT_WAY; way++)
                    c_delta[set][way] >>= 1;
                c_sig[set] >>= 1;
            }

            SPP_DP(std::cout << "[PT] " << __func__ << " hit sig: " << hex << last_sig << dec << " set: " << set
                             << " way: "
                             << match;
                           std::cout << " delta: " << delta[set][match] << " c_delta: " << c_delta[set][match]
                                     << " c_sig: "
                                     << c_sig[set] << std::endl;);

            break;
        }
    }

    // Case 2: Miss
    if (match == PT_WAY) {
        uint32_t victim_way = PT_WAY, min_counter = C_SIG_MAX;

        for (match = 0; match < PT_WAY; match++) {
            if (c_delta[set][match] < min_counter) { // Select an entry with the minimum c_delta
                victim_way = match;
                min_counter = c_delta[set][match];
            }
        }

        delta[set][victim_way] = curr_delta;
        c_delta[set][victim_way] = 0;
        c_sig[set]++;
        if (c_sig[set] > C_SIG_MAX) {
            for (uint32_t way = 0; way < PT_WAY; way++)
                c_delta[set][way] >>= 1;
            c_sig[set] >>= 1;
        }

        SPP_DP(std::cout << "[PT] " << __func__ << " miss sig: " << hex << last_sig << dec << " set: " << set
                         << " way: "
                         << victim_way;
                       std::cout << " delta: " << delta[set][victim_way] << " c_delta: " << c_delta[set][victim_way]
                                 << " c_sig: " << c_sig[set] << std::endl;);

#ifdef SPP_SANITY_CHECK
        // Assertion
        if (victim_way == PT_WAY) {
            std::cout << "[PT] Cannot find a replacement victim!" << std::endl;
            assert(0);
        }
#endif
    }
}

void PATTERN_TABLE::read_pattern(uint32_t curr_sig, int *delta_q, uint32_t *confidence_q, uint32_t &lookahead_way,
                                 uint32_t &lookahead_conf,
                                 uint32_t &pf_q_tail, uint32_t &depth) {
    // Update (sig, delta) correlation
    uint32_t set = get_hash(curr_sig) % PT_SET, local_conf = 0, pf_conf = 0, max_conf = 0;

    if (c_sig[set]) {
        for (uint32_t way = 0; way < PT_WAY; way++) {
            local_conf = (100 * c_delta[set][way]) / c_sig[set];
            pf_conf = depth ? (GHR.global_accuracy * c_delta[set][way] / c_sig[set] * lookahead_conf / 100)
                            : local_conf;

            if (pf_conf >= PF_THRESHOLD) {
                confidence_q[pf_q_tail] = pf_conf;
                delta_q[pf_q_tail] = delta[set][way];

                // Lookahead path follows the most confident entry
                if (pf_conf > max_conf) {
                    lookahead_way = way;
                    max_conf = pf_conf;
                }
                pf_q_tail++;

                SPP_DP(std::cout << "[PT] " << __func__ << " HIGH CONF: " << pf_conf << " sig: " << hex << curr_sig
                                 << dec
                                 << " set: " << set << " way: " << way;
                               std::cout << " delta: " << delta[set][way] << " c_delta: " << c_delta[set][way]
                                         << " c_sig: "
                                         << c_sig[set];
                               std::cout << " conf: " << local_conf << " depth: " << depth << std::endl;);
            } else {
                SPP_DP(std::cout << "[PT] " << __func__ << "  LOW CONF: " << pf_conf << " sig: " << hex << curr_sig
                                 << dec
                                 << " set: " << set << " way: " << way;
                               std::cout << " delta: " << delta[set][way] << " c_delta: " << c_delta[set][way]
                                         << " c_sig: "
                                         << c_sig[set];
                               std::cout << " conf: " << local_conf << " depth: " << depth << std::endl;);
            }
        }
        lookahead_conf = max_conf;
        if (lookahead_conf >= PF_THRESHOLD)
            depth++;

        SPP_DP(std::cout << "global_accuracy: " << GHR.global_accuracy << " lookahead_conf: " << lookahead_conf
                         << std::endl;);
    } else
        confidence_q[pf_q_tail] = 0;
}

bool PREFETCH_FILTER::check(uint64_t check_addr, FILTER_REQUEST filter_request) {
    uint64_t cache_line = check_addr >> LOG2_BLOCK_SIZE;
    uint64_t hash = get_hash(cache_line);
    uint64_t quotient = (hash >> REMAINDER_BIT) & ((1 << QUOTIENT_BIT) - 1);
    uint64_t remainder = hash % (1 << REMAINDER_BIT);

    /*
    SPP_DP(std::cout << "[FILTER] check_addr: " << hex << check_addr << " check_cache_line: "
                     << (check_addr >> LOG2_BLOCK_SIZE);
                   std::cout << " hash: " << hash << dec << " quotient: " << quotient
                   << " remainder: " << remainder << std::endl;);
    */

    switch (filter_request) {
        case SPP_L2C_PREFETCH:
            if ((valid[quotient] || useful[quotient]) && remainder_tag[quotient] == remainder) {
                /*
                SPP_DP(std::cout << "[FILTER] " << __func__ << " line is already in the filter check_addr: " << hex
                                 << check_addr << " cache_line: " << cache_line << dec;
                               std::cout << " quotient: " << quotient << " valid: " << valid[quotient] << " useful: "
                                         << useful[quotient] << std::endl;);
                */
                return false; // False return indicates "Do not prefetch"
            } else {
                valid[quotient] = 1;  // Mark as prefetched
                useful[quotient] = 0; // Reset useful bit
                remainder_tag[quotient] = remainder;
                /*
                SPP_DP(std::cout    << "[FILTER] " << __func__ << " set valid for check_addr: " << hex << check_addr
                                    << " cache_line: " << cache_line << dec;
                                    std::cout << " quotient: " << quotient << " remainder_tag: "
                                    << remainder_tag[quotient] << " valid: " << valid[quotient]
                                    << " useful: " << useful[quotient] << std::endl;);
                */
            }
            break;

        case SPP_LLC_PREFETCH:
            if ((valid[quotient] || useful[quotient]) && remainder_tag[quotient] == remainder) {
                /*
                SPP_DP(std::cout << "[FILTER] " << __func__ << " line is already in the filter check_addr: " << hex
                                 << check_addr << " cache_line: " << cache_line << dec;
                               std::cout << " quotient: " << quotient << " valid: " << valid[quotient] << " useful: "
                                         << useful[quotient] << std::endl;);
                */

                return false; // False return indicates "Do not prefetch"
            } else {
                /*
                // NOTE: SPP_LLC_PREFETCH has relatively low confidence (FILL_THRESHOLD <=
                // SPP_LLC_PREFETCH < PF_THRESHOLD) Therefore, it is safe to prefetch this
                // cache line in the large LLC and save precious L2C capacity If this
                // prefetch request becomes more confident and SPP eventually issues
                // SPP_L2C_PREFETCH, we can get this cache line immediately from the LLC
                // (not from DRAM) To allow this fast prefetch from LLC, SPP does not set
                // the valid bit for SPP_LLC_PREFETCH

                // valid[quotient] = 1;
                // useful[quotient] = 0;

                SPP_DP(std::cout << "[FILTER] " << __func__ << " don't set valid for check_addr: " << hex << check_addr
                                 << " cache_line: " << cache_line << dec;
                               std::cout << " quotient: " << quotient << " valid: " << valid[quotient] << " useful: "
                                         << useful[quotient] << std::endl;);
                */
            }
            break;

        case L2C_DEMAND:
            if ((remainder_tag[quotient] == remainder) && (useful[quotient] == 0)) {
                useful[quotient] = 1;
                if (valid[quotient])
                    GHR.pf_useful++;
                /*
                // This cache line was prefetched by SPP and actually
                // used in the program
                SPP_DP(std::cout << "[FILTER] " << __func__ << " set useful for check_addr: " << hex << check_addr
                                 << " cache_line: " << cache_line << dec;
                               std::cout << " quotient: " << quotient << " valid: " << valid[quotient] << " useful: "
                                         << useful[quotient];
                               std::cout << " GHR.pf_issued: " << GHR.pf_issued << " GHR.pf_useful: " << GHR.pf_useful
                                         << std::endl;);
                */
            }
            break;

        case L2C_EVICT:
            // Decrease global pf_useful counter when there is a useless prefetch
            // (prefetched but not used)
            if (valid[quotient] && !useful[quotient] && GHR.pf_useful)
                GHR.pf_useful--;
            // Reset filter entry
            valid[quotient] = 0;
            useful[quotient] = 0;
            remainder_tag[quotient] = 0;
            break;

        default:
            // Assertion
            std::cout << "[FILTER] Invalid filter request type: " << filter_request << std::endl;
            assert(0);
    }

    return true;
}

void GLOBAL_REGISTER::update_entry(uint32_t pf_sig, uint32_t pf_confidence, uint32_t pf_offset, int pf_delta) {
    // NOTE: GHR implementation is slightly different from the original paper
    // Instead of matching (last_offset + delta), GHR simply stores and matches
    // the pf_offset
    uint32_t min_conf = 100, victim_way = MAX_GHR_ENTRY;

    SPP_DP(std::cout << "[GHR] Crossing the page boundary pf_sig: " << hex << pf_sig << dec;
                   std::cout << " confidence: " << pf_confidence << " pf_offset: " << pf_offset << " pf_delta: "
                             << pf_delta
                             << std::endl;);

    for (uint32_t i = 0; i < MAX_GHR_ENTRY; i++) {
        // if (sig[i] == pf_sig) { // TODO: Which one is better and consistent?
        // If GHR already holds the same pf_sig, update the GHR entry with the
        // latest info
        if (valid[i] && (offset[i] == pf_offset)) {
            // If GHR already holds the same pf_offset, update the GHR entry with the
            // latest info
            sig[i] = pf_sig;
            confidence[i] = pf_confidence;
            // offset[i] = pf_offset;
            delta[i] = pf_delta;

            SPP_DP(std::cout << "[GHR] Found a matching index: " << i << std::endl;);

            return;
        }

        // GHR replacement policy is based on the stored confidence value
        // An entry with the lowest confidence is selected as a victim
        if (confidence[i] < min_conf) {
            min_conf = confidence[i];
            victim_way = i;
        }
    }

    // Assertion
    if (victim_way >= MAX_GHR_ENTRY) {
        std::cout << "[GHR] Cannot find a replacement victim!" << std::endl;
        assert(0);
    }

    SPP_DP(std::cout << "[GHR] Replace index: " << victim_way << " pf_sig: " << hex << sig[victim_way] << dec;
                   std::cout << " confidence: " << confidence[victim_way] << " pf_offset: " << offset[victim_way]
                             << " pf_delta: " << delta[victim_way] << std::endl;);

    valid[victim_way] = 1;
    sig[victim_way] = pf_sig;
    confidence[victim_way] = pf_confidence;
    offset[victim_way] = pf_offset;
    delta[victim_way] = pf_delta;
}

uint32_t GLOBAL_REGISTER::check_entry(uint32_t page_offset) {
    uint32_t max_conf = 0, max_conf_way = MAX_GHR_ENTRY;

    for (uint32_t i = 0; i < MAX_GHR_ENTRY; i++) {
        if ((offset[i] == page_offset) && (max_conf < confidence[i])) {
            max_conf = confidence[i];
            max_conf_way = i;
        }
    }

    return max_conf_way;
}