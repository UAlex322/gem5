#ifndef __MEM_CACHE_TAGS_ADAPTIVE_CACHE_ADAPTIVE_ASSOC_HH__
#define __MEM_CACHE_TAGS_ADAPTIVE_CACHE_ADAPTIVE_ASSOC_HH__

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "base/logging.hh"
#include "base/types.hh"
#include "cpu/base.hh"
#include "debug/AdaptiveAssoc.hh"
#include "mem/cache/base.hh"
#include "mem/cache/cache.hh"
#include "mem/cache/cache_blk.hh"
#include "mem/cache/replacement_policies/base.hh"
#include "mem/cache/replacement_policies/replaceable_entry.hh"
#include "mem/cache/tags/base.hh"
#include "mem/cache/tags/base_set_assoc.hh"
#include "mem/cache/tags/indexing_policies/base.hh"
#include "mem/cache/tags/partitioning_policies/partition_manager.hh"
#include "mem/packet.hh"
#include "params/AdaptiveAssoc.hh"
#include "sim/clocked_object.hh"
#include "sim/cur_tick.hh"
#include "sim/eventq.hh"

namespace gem5
{

class System;
class AdaptiveAssoc : public BaseSetAssoc
{
  protected:
    // Parameters of Cache at time (100%)
    struct CacheFeatures
    {

        // Miss rate %
        double miss_rate;

        // Number of misses
        uint64_t miss_count;

        // Total mem access
        uint64_t total_mem_access;

        // Instructions per cycle
        double ipc;

        // Previous associativity
        unsigned prev_assoc;

        CacheFeatures(double _miss_rate = 0.0, uint64_t _miss_count = 0,
                      uint64_t _total_memory_access = 0, double _ipc = 0.0,
                      unsigned _prev_assoc = 16);
    };

    // Decision Tree (100%)
    class DecisionTree
    {
      public:
        unsigned Predict(const CacheFeatures &f) const;
    };

    // Monitor that collects info about cache, ipc etc.
    class PerformanceMonitor
    {
        // Number of instructions
        uint64_t instructions;
        // Number of access to mem
        uint64_t mem_accesses;
        // Number of cache misses
        uint64_t cache_misses;

        // Pointer to cache
        AdaptiveAssoc *cache_tag;

        // How much Tick in one Clock of processor
        Tick proc_time_clock;
        // Tick when reconfig period start
        Tick period_start;

        // Time of one reconfiguration period
        uint64_t reconfig_period;
        // Time is spent to decision making period
        uint64_t decision_period;

        // Last cache info
        CacheFeatures features;

        uint64_t executedInsts();

      public:
        friend AdaptiveAssoc;

        PerformanceMonitor(AdaptiveAssoc *_cache_tag,
                           uint64_t _reconfig_period = 30000000,
                           Tick _proc_time_clock = 1000);

        // setCpuClock
        void setCpuClock(Tick tm);

        // Access to mem had happened
        void onAccess(bool hit);

        // Start new reconfiguration period
        void startNewPeriod(Tick now);

        // Call when decision-making event is over
        void processNextDecisionEndEvent();

        // Call when reconfiguration period is over
        void processPeriodEndEvent();
    };

    EventFunctionWrapper nextDecisionEndEvent;
    EventFunctionWrapper nextPeriodEndEvent;

    // Decision Tree
    DecisionTree decision_tree;
    // Monitor that does many work
    PerformanceMonitor monitor;
    // Pointer to cache
    BaseCache *parent_cache;
    // Vector of cores
    std::vector<BaseCPU *> cpus;

    // Current associativity
    unsigned current_assoc;
    // Time of reconfiguration period
    uint64_t reconfig_period;

  public:
    AdaptiveAssoc(const AdaptiveAssocParams &p);
    virtual ~AdaptiveAssoc() {}

    // Access to cache
    CacheBlk *accessBlock(const PacketPtr pkt, Cycles &lat) override;

    // Change associativity
    void reconfigureAssociativity(unsigned new_assoc);

    // Get current associativity
    unsigned getCurrentAssoc() const;
    // Get amount of reconfigurations
    uint64_t getReconfigPeriod() const;
    // Set things after all simobjects initialized
    void init() override;
    // Find victim
    CacheBlk *findVictim(const CacheBlk::KeyType &key, const std::size_t size,
                         std::vector<CacheBlk *> &evict_blks,
                         const uint64_t partition_id = 0) override;

  protected:
    // Writeback to memory
    void writebackDirtyBlocks();
    // Clear cache
    void flushCache();
};

} // namespace gem5

#endif // __MEM_CACHE_TAGS_ADAPTIVE_CACHE_ADAPTIVE_ASSOC_HH__
