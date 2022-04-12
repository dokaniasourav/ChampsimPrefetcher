#include <iostream>
#include <algorithm>
#include <array>
#include <map>

#include "cache.h"

constexpr int PREFETCH_DEGREE = 5;	//degree of prefetch

struct lookahead_entry {
  uint64_t address = 0;
  int degree = 0; 			// degree remaining
};

std::map<CACHE*, lookahead_entry> lookahead;

void CACHE::prefetcher_initialize() 
{ 
  std::cout << NAME << " next line degree prefetcher" << std::endl; 
}

void CACHE::prefetcher_cycle_operate() 
{
  // If a lookahead is active
  if (auto [old_pf_address, degree] = lookahead[this]; degree > 0) 
  {
    auto pf_address = old_pf_address + (1 << LOG2_BLOCK_SIZE);
    // If the next step would exceed the degree or run off the page, stop
    if (virtual_prefetch || (pf_address >> LOG2_PAGE_SIZE) == (old_pf_address >> LOG2_PAGE_SIZE)) 
    {
      // check the MSHR occupancy to decide if we're going to prefetch to this level or not
      //bool success = prefetch_line(pf_address, true, 0);
      bool success = prefetch_line(pf_address, (get_occupancy(0, pf_address) < get_size(0, pf_address) / 2), 0);
      if (success)
        lookahead[this] = {pf_address, degree - 1};
      // If we fail, try again next cycle
    } 
    else 
    {
      lookahead[this] = {};
    }
  }
}


uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in)
{
  //uint64_t pf_addr = addr + (1 << LOG2_BLOCK_SIZE);
  lookahead[this] = {addr, PREFETCH_DEGREE};
  //prefetch_line(ip, addr, pf_addr, true, 0);
  return metadata_in;
}

uint32_t CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
  return metadata_in;
}


void CACHE::prefetcher_final_stats() {}
