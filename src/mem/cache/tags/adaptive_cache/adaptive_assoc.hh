#ifndef __MEM_CACHE_TAGS_ADAPTIVE_CACHE_ADAPTIVE_ASSOC_HH__
#define __MEM_CACHE_TAGS_ADAPTIVE_CACHE_ADAPTIVE_ASSOC_HH__

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "base/logging.hh"
#include "base/types.hh"
#include "cpu/base.hh"
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
#include "sim/clocked_object.hh"
#include "sim/cur_tick.hh"
#include "sim/eventq.hh"

#include "mem/cache/tags/base_set_assoc.hh"
#include "params/AdaptiveAssoc.hh"
#include "debug/AdaptiveAssoc.hh"


namespace gem5 {

  protected:

    class System;
    class AdaptiveAssoc : public BaseSetAssoc
    {   

        // Parameters of Cache at time (100%)
        struct CacheFeatures {

            double miss_rate;            // Miss rate %
            uint64_t miss_count;         // Number of misses
            uint64_t total_mem_access;   // Total mem access
            double ipc;                  // Instructions per cycle
            uint64_t prev_assoc;         // Previous associativity

            CacheFeatures(
                double _miss_rate = 0.0, 
                uint64_t _miss_count = 0, 
                uint64_t _total_memory_access = 0,
                double _ipc = 0.0, 
                uint64_t _prev_assoc = 16
            );
        };

        // Decision Tree (100%)
        class DecisionTree
        {
        public:
            uint64_t Predict(const CacheFeatures& f) const;
        };

        // Monitor that collects info about cache, ipc etc.
        class PerformanceMonitor : public ClockedObject
        {
            EventFunctionWrapper nextDecisionEvent;
            EventFunctionWrapper nextPeriodEndEvent;

            uint64_t instructions;      // Number of instructions
            uint64_t mem_accesses;      // Number of access to mem
            uint64_t cache_misses;      // Number of cache misses

            AdaptiveAssoc* cache_tag;   // Pointer to cache

            Tick proc_time_clock;         // How much Tick in one Clock of processor
            Tick period_start;

            uint64_t reconfig_period;   // Time of one reconfiguration period
            uint64_t decision_period;   // Time is spend to decision making period

            bool in_decision_phase;     // Is decision making period phase

            CacheFeatures features;    // Last cache info

            void processNextDecisionEvent();
            void processPeriodEndEvent();

        public:

            PerformanceMonitor(
                AdaptiveAssoc* _cache_tag, 
                uint64_t _reconfig_period = 30000000, 
                Tick _proc_time_clock = 1000
            );

            // setCpuClock
            void setCpuClock(Tick tm) {
                proc_time_clock = tm;
            }

            // Access to mem had happened
            void onAccess(bool hit) {
                mem_accesses++;
                if (!hit)
                    cache_misses++;
            }

            void startNewPeriod(Tick now) {
                period_start = now;
                instructions = 0;
                mem_accesses = 0;
                cache_misses = 0;
                in_decision_phase = true;

                if (nextDecisionEvent.scheduled()) deschedule(nextDecisionEvent);
                schedule(nextDecisionEvent, now + decision_period);

                if (nextPeriodEndEvent.scheduled()) deschedule(nextPeriodEndEvent);
                schedule(nextPeriodEndEvent, now + reconfig_period);
            }

            /*bool isDecisionTime(Tick now) const {
                if (!in_decision_phase)
                    return false;
                return (now - period_start) >= decision_period;
            }

            void endDecisionPhase(Tick now, uint64_t prev_assoc) {
                uint64_t cycles = (now - period_start) / procTimeClock;
                double ipc_val = (cycles > 0) ? (double)instructions / cycles : 0.0;

                double miss_rate_pct = (mem_accesses > 0) ?
                ((double)cache_misses / mem_accesses) * 100.0 : 0.0;

                // Save last features
                last_features = CacheFeatures(
                    miss_rate_pct, cache_misses, mem_accesses, ipc_val, prev_assoc);
                features_ready = true;

                instructions = 0;
                mem_accesses = 0;
                cache_misses = 0;
                in_decision_phase = false;  // Start of Stable period
            }

            // getFeatures
            bool getFeatures(CacheFeatures& f) const {
                if (!features_ready)
                    return false;
                f = last_features;
                return true;
            }

            // Is Reconfiguration period ended
            bool isPeriodEnd(Tick now) const {
                return (now - period_start) >= reconfig_period;
            }

            // Start new period
            void startNewPeriod(Tick now) {
                period_start = now;
                instructions = 0;
                mem_accesses = 0;
                cache_misses = 0;
                in_decision_phase = true;
                features_ready = false;

                if (nextDecisionEvent.scheduled()) deschedule(nextDecisionEvent);
                schedule(nextDecisionEvent, now + decision_period);

                if (nextPeriodEndEvent.scheduled()) deschedule(nextPeriodEndEvent);
                schedule(nextPeriodEndEvent, now + reconfig_period);
            }

            // Set null values
            void reset() {
                instructions = 0;
                mem_accesses = 0;
                cache_misses = 0;
                features_ready = false;
                in_decision_phase = true;
            }

            uint64_t getMemAccesses() const { return mem_accesses; }
            uint64_t getCacheMisses() const { return cache_misses; }
            double getMissRatePct() const {
                return (mem_accesses > 0) ?
                ((double)cache_misses / mem_accesses) * 100.0 : 0.0;
            }*/
        };

        DecisionTree decision_tree;      // Decision Tree
        PerformanceMonitor monitor;     // Monitor that does many work
        BaseCache* parent_cache;        // Pointer to cache
        std::vector<BaseCPU*> cpus;     // Vector of cores

        unsigned current_assoc;     // Current associativity
        uint64_t reconfig_period;   // Time of reconfiguration period

  public:
        AdaptiveAssoc(const AdaptiveAssocParams &p);
        virtual ~AdaptiveAssoc() {}

        CacheBlk* accessBlock(const PacketPtr pkt, Cycles &lat) override;   // Access to cache
        void reconfigureAssociativity(unsigned new_assoc);                  // Change associativity
        unsigned getCurrentAssoc() const { return current_assoc; }          // Get current associativity
        uint64_t getReconfigCount() const { return reconfig_count; }        // Get amount of reconfigurations
        void init() override;                                               // Set things after all simobjects initialized
        CacheBlk* findVictim(const CacheBlk::KeyType& key,
                             const std::size_t size,
                             std::vector<CacheBlk*>& evict_blks,
                             const uint64_t partition_id=0) override;       // Find victim

  protected:

        void writebackDirtyBlocks();    // Writeback to memory
        void flushCache();              // Clear cache
    };

} // namespace gem5

#endif // __MEM_CACHE_TAGS_ADAPTIVE_CACHE_ADAPTIVE_ASSOC_HH__

