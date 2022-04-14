/***
 * THIS FILE IS AUTOMATICALLY GENERATED
 * Do not edit this file. It will be overwritten when the configure script is run.
 ***/

#include "cache.h"
#include "champsim.h"
#include "dram_controller.h"
#include "ooo_cpu.h"
#include "ptw.h"
#include "vmem.h"
#include "operable.h"
#include "champsim_constants.h"
#include <array>
#include <vector>
VirtualMemory vmem(8589934592, 1 << 12, 5, 1, 200);

MEMORY_CONTROLLER DRAM(1.25);
CACHE LLC("LLC", 1.0, 6, 2048, 16, 32, 32, 32, 64, 19, 1, 1, 1, LOG2_BLOCK_SIZE, 0, 0, 0, 5, &DRAM, CACHE::pref_t::pprefetcherDno, CACHE::repl_t::rreplacementDlru);
CACHE cpu0_L2C("cpu0_L2C", 1.0, 5, 1024, 8, 32, 32, 16, 32, 9, 1, 1, 1, LOG2_BLOCK_SIZE, 0, 0, 0, 5, &LLC, CACHE::pref_t::pprefetcherDppf_spp_dev, CACHE::repl_t::rreplacementDlru);
CACHE cpu0_L1D("cpu0_L1D", 1.0, 4, 64, 12, 64, 64, 8, 16, 4, 1, 2, 2, LOG2_BLOCK_SIZE, 0, 1, 0, 5, &cpu0_L2C, CACHE::pref_t::pprefetcherDno, CACHE::repl_t::rreplacementDlru);
PageTableWalker cpu0_PTW("cpu0_PTW", 0, 3, 1, 2, 1, 4, 2, 4, 4, 8, 16, 5, 2, 2, 0, &cpu0_L1D);
CACHE cpu0_STLB("cpu0_STLB", 1.0, 2, 1024, 8, 32, 32, 16, 32, 9, 1, 1, 1, LOG2_PAGE_SIZE, 0, 0, 0, 5, &cpu0_PTW, CACHE::pref_t::pprefetcherDno, CACHE::repl_t::rreplacementDlru);
CACHE cpu0_L1I("cpu0_L1I", 1.0, 1, 64, 8, 64, 64, 32, 8, 3, 1, 2, 2, LOG2_BLOCK_SIZE, 0, 1, 1, 5, &cpu0_L2C, CACHE::pref_t::CPU_REDIRECT_pprefetcherDno_instr_, CACHE::repl_t::rreplacementDlru);
CACHE cpu0_ITLB("cpu0_ITLB", 1.0, 1, 16, 4, 16, 16, 0, 8, 0, 1, 2, 2, LOG2_PAGE_SIZE, 0, 1, 1, 5, &cpu0_STLB, CACHE::pref_t::pprefetcherDno, CACHE::repl_t::rreplacementDlru);
CACHE cpu0_DTLB("cpu0_DTLB", 1.0, 1, 16, 4, 16, 16, 0, 8, 0, 1, 2, 2, LOG2_PAGE_SIZE, 0, 1, 0, 5, &cpu0_STLB, CACHE::pref_t::pprefetcherDno, CACHE::repl_t::rreplacementDlru);
O3_CPU cpu0(0, 1.0, 32, 8, 16, 64, 32, 32, 352, 128, 72, 6, 6, 6, 128, 4, 2, 2, 5, 1, 1, 1, 0, 0, &cpu0_ITLB, &cpu0_DTLB, &cpu0_L1I, &cpu0_L1D, O3_CPU::bpred_t::bbranchDbimodal1, O3_CPU::btb_t::bbtbDbasic_btb, O3_CPU::ipref_t::pprefetcherDno_instr);
std::array<O3_CPU*, NUM_CPUS> ooo_cpu {
&cpu0
};
std::array<CACHE*, NUM_CACHES> caches {
&LLC, &cpu0_L2C, &cpu0_L1D, &cpu0_STLB, &cpu0_L1I, &cpu0_ITLB, &cpu0_DTLB
};
std::array<champsim::operable*, NUM_OPERABLES> operables {
&cpu0, &LLC, &cpu0_L2C, &cpu0_L1D, &cpu0_PTW, &cpu0_STLB, &cpu0_L1I, &cpu0_ITLB, &cpu0_DTLB, &DRAM
};
