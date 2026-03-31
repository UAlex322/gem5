#ifndef __MEM_CACHE_TAGS_ADAPTIVE_CACHE_ADAPTIVE_ASSOC_HH__
#define __MEM_CACHE_TAGS_ADAPTIVE_CACHE_ADAPTIVE_ASSOC_HH__

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "base/logging.hh"
#include "base/types.hh"
#include "mem/cache/cache_blk.hh"
#include "mem/cache/replacement_policies/base.hh"
#include "mem/cache/replacement_policies/replaceable_entry.hh"
#include "mem/cache/tags/base.hh"
#include "mem/cache/tags/indexing_policies/base.hh"
#include "mem/cache/tags/partitioning_policies/partition_manager.hh"
#include "mem/packet.hh"
#include "params/BaseSetAssoc.hh"

#include "mem/cache/tags/base_set_assoc.hh"
#include "params/AdaptiveAssoc.hh"
#include "debug/AdaptiveAssoc.hh"
#include "sim/cur_tick.hh"
#include "sim/eventq.hh"

namespace gem5 {

    class System;
    class AdaptiveAssoc : public BaseSetAssoc
    {
    private:

        // Parameters of Cache at time

        struct CacheFeatures {
            double miss_rate_pct;        // Miss rate %
            uint64_t miss_count;         // Number of misses
            uint64_t total_mem_access;   // Total mem access
            double ipc;                  // Instructions per cycle
            uint64_t prev_assoc;         // Previous associativity

            CacheFeatures() : miss_rate_pct(0.0), miss_count(0),
                total_mem_access(0), ipc(0.0), prev_assoc(16) {}

            CacheFeatures(double mr_pct, uint64_t mc, uint64_t tma,
                          double i, uint64_t pa) :
                miss_rate_pct(mr_pct), miss_count(mc), total_mem_access(tma), ipc(i), prev_assoc(pa) {}
        };

        // Decision Tree

        class DecisionTree
        {
        public:
            DecisionTree() = default;

            int predict(const CacheFeatures& f) const
            {
                // Ready variant

                if (f.miss_rate_pct < 0.27) {   // Miss rate % < 0.27
                    switch (f.prev_assoc) {
                        case 1:  return 1;
                        case 2:  return 2;
                        case 4:  return 4;
                        case 8:  return 8;
                        case 16: return 16;
                        default: return 16;
                    }
                }
                else if (f.miss_rate_pct < 0.48) {
                    if (f.total_mem_access < 5833) {
                        return 2;
                    }
                    else if (f.total_mem_access < 9241) {
                        if (f.prev_assoc == 1 || f.prev_assoc == 2 || f.prev_assoc == 4) {
                            return 1;
                        } else if (f.prev_assoc == 8) {
                            return 4;
                        } else {
                            return 8;
                        }
                    }
                    else {
                        if (f.miss_count < 19441) {
                            return 4;
                        } else if (f.miss_count < 603810) {
                            return 8;
                        } else {
                            return 16;
                        }
                    }
                }
                else if (f.miss_rate_pct <= 2.52) {
                    if (f.ipc < 41895) {
                        return 4;
                    } else if (f.ipc < 820036) {
                        return 8;
                    } else {
                        return 16;
                    }
                }
                else {
                    return 16;
                }
            }

        };

        // Monitor that collects info about cache, ipc etc.

        class PerformanceMonitor
        {
        private:

            uint64_t instructions;      // Number of instructions
            uint64_t mem_accesses;      // Number of access to mem
            uint64_t cache_misses;      // Number of cache misses

            AdaptiveAssoc* cache_tag;   // Pointer to cache

            Tick procTimeClock;         // How much Tick in one Clock of processor
            Tick period_start;          // Time when began current reconfiguration period

            uint64_t reconfig_period;   // Time of one reconfiguration period
            uint64_t decision_period;   // Time is spend to decision making period

            bool in_decision_phase;     // Is decision making period phase
            bool features_ready;        // Cache features are ready


            CacheFeatures last_features;    // Last cache info

        public:
            PerformanceMonitor(uint64_t period = 30000000, Tick _procTimeClock = 1000) : instructions(0), mem_accesses(0), cache_misses(0), cache_tag(nullptr),
                procTimeClock(_procTimeClock), period_start(0), reconfig_period(period), decision_period(period / 10),
                in_decision_phase(true), features_ready(false) {}

            // setCpuClock

            void setCpuClock(Tick tm) {
                procTimeClock = tm;
            }

            void onAccess(bool hit) {           // Access to mem had happened
                if (in_decision_phase) {
                    mem_accesses++;
                    if (!hit)
                        cache_misses++;
                }
            }

            void onInstruction() {              // Access to instruction
                if (in_decision_phase) instructions++;
            }

            bool isDecisionTime(Tick now) const {
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
            }
        };

        DecisionTree decisionTree;      // Decision Tree
        PerformanceMonitor monitor;     // Monitor that does many work

        unsigned current_assoc;     // Current associativity
        unsigned target_assoc;      // Target associativity
        bool need_reconfig;         // Is reconfig should start
        bool reconfig_in_progress;  // Is reconfiguration period
        uint64_t reconfig_cycles_left;  // Cycles until end
        uint64_t cycle_counter;     // Cycle counter
        uint64_t reconfig_count;    // Amount of reconfigurations
        uint64_t reconfig_period;   // Time of reconfiguration period
        uint64_t reconfig_overhead; // Reconfiguration overhead

    public:
        AdaptiveAssoc(const AdaptiveAssocParams &p);
        virtual ~AdaptiveAssoc() {}

        CacheBlk* accessBlock(const PacketPtr pkt, Cycles &lat) override;   // Access to cache
        void updateCycle(Tick now);                                         // Update state of cache
        void reconfigureAssociativity(unsigned new_assoc);                  // Change associativity
        unsigned getCurrentAssoc() const { return current_assoc; }          // Get current associativity
        uint64_t getReconfigCount() const { return reconfig_count; }        // Get amount of reconfigurations
        void init() override;                                               // Set things after all simobjects initialized
        CacheBlk* findVictim(const CacheBlk::KeyType& key,
                             const std::size_t size,
                             std::vector<CacheBlk*>& evict_blks,
                             const uint64_t partition_id=0) override;       // Find victim

    private:

        void writebackDirtyBlocks();    // Writeback to memory
        void flushCache();              // Clear cache
    };

} // namespace gem5

#endif // __MEM_CACHE_TAGS_ADAPTIVE_CACHE_ADAPTIVE_ASSOC_HH__

