#include "mem/cache/tags/adaptive_cache/adaptive_assoc.hh"
#include "mem/cache/tags/indexing_policies/set_associative.hh"
#include "sim/system.hh"
#include "cpu/base.hh"

namespace gem5 
{

    AdaptiveAssoc::CacheFeatures::CacheFeatures(
        double _miss_rate, 
        uint64_t _miss_count, 
        uint64_t _total_memory_access,
        double _ipc, 
        uint64_t _prev_assoc
    ) :
        miss_rate(_miss_rate), 
        miss_count(_miss_count), 
        total_mem_access(_total_memory_access), 
        ipc(_ipc), 
        prev_assoc(_prev_assoc) 
        {}

    uint64_t 
    AdaptiveAssoc::DecisionTree::Predict(const CacheFeatures& f) const 
    {
        if (f.miss_rate < 0.27) {
            switch (f.prev_assoc) {
                case 1:  return 1;
                case 2:  return 2;
                case 4:  return 4;
                case 8:  return 8;
                case 16: return 16;
                default: return 16;
            }
        }
        else if (f.miss_rate < 0.48) {
            if (f.total_mem_access < 5833) {
                return 2;
            }
            else if (f.total_mem_access < 9241) {
                switch (f.prev_assoc) {
                    case 1: 
                    case 2: 
                    case 4: 
                        return 1;
                    case 8:
                        return 4;
                    default:
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
        else if (f.miss_rate <= 2.52) {
            if (f.ipc < 41895) {
                return 4;
            } else if (f.ipc < 820036) {
                return 8;
            } else {
                return 16;
            }
        }
        return 16;
    }

  AdaptiveAssoc::PerformanceMonitor::PerformanceMonitor(
        AdaptiveAssoc* _cache_tag, 
        uint64_t _reconfig_period, 
        Tick _proc_time_clock
    ) :
        nextDecisionEvent([this](){ processNextDecisionEvent(); }, name() + "nextDecisionEvent"),
        nextPeriodEndEvent([this](){ processPeriodEndEvent(); }, name() + "nextPeriodEndEvent"),
        instructions(0), 
        mem_accesses(0), 
        cache_misses(0), 
        cache_tag(_cache_tag),
        proc_time_clock(_proc_time_clock), 
        period_start(0), 
        reconfig_period(_reconfig_period), 
        decision_period(_reconfig_period / 10),
        in_decision_phase(true), 
        {}

    AdaptiveAssoc::AdaptiveAssoc(const AdaptiveAssocParams &p) : 
        BaseSetAssoc(p),
        parent_cache(p.parent_cache),
        cpus(p.cpus),
        monitor(this, p.reconfig_period),
        current_assoc(16),
        reconfig_period(p.reconfig_period),
    {
        DPRINTF(AdaptiveAssoc, "Adaptive cache initialized.\n"
            "  Reconfig period: %d cycles\n"
            "  Initial associativity: %d-way\n"
            "  Decision period: %d cycles (10%%)\n",
                reconfig_period,
                current_assoc,
                reconfig_period / 10
        );
    }

    void AdaptiveAssoc::init() {
        BaseSetAssoc::init();
        BaseCPU* cpu = cpus[0];
        monitor.setCpuClock(cpu->clockPeriod());
        monitor.startNewPeriod(cpu->clockEdge());
    }

    CacheBlk*
    AdaptiveAssoc::accessBlock(const PacketPtr pkt, Cycles &lat)
    {
        CacheBlk* blk = BaseSetAssoc::accessBlock(pkt, lat);
        bool hit = (blk != nullptr && blk->isValid());
        monitor.onAccess(hit);
        return blk;
    }

    void AdaptiveAssoc::reconfigureAssociativity(unsigned new_assoc)
    {
        if (new_assoc == current_assoc) return;

        DPRINTF(AdaptiveAssoc, "Starting reconfiguration: %d-way -> %d-way\n",
                current_assoc, new_assoc);

        writebackDirtyBlocks();
        flushCache();
        setWayAllocationMax(new_assoc);
        current_assoc = new_assoc;

        DPRINTF(AdaptiveAssoc, "Reconfiguration started. %d cycles remaining\n",
                reconfig_cycles_left);
    }

    void AdaptiveAssoc::writebackDirtyBlocks()
    {
        int dirty_count = 0;

        // Find dirty blocks

        for (auto& blk : blks) {
            if (blk.isSet(CacheBlk::DirtyBit)) {
                Addr blk_addr = regenerateBlkAddr(&blk);

                DPRINTF(AdaptiveAssoc, "Writeback: addr=%#lx\n", blk_addr);
                RequestPtr req = std::make_shared<Request>(
                    blk_addr, blkSize, 0, Request::wbRequestorId);
                if (blk.isSecure()) {
                    req->setFlags(Request::SECURE);
                }

                PacketPtr pkt = new Packet(req, MemCmd::WriteReq);
                pkt->dataStatic(blk.data);
                system->getPhysMem().access(pkt);
                blk.clearCoherenceBits(CacheBlk::DirtyBit);
                dirty_count++;

                delete pkt;
            }
        }

        DPRINTF(AdaptiveAssoc, "Writeback completed. %d dirty blocks written.\n",
                dirty_count);
    }

    void AdaptiveAssoc::flushCache()
    {
        int invalidated = 0;
        for (auto& blk : blks) {
            if (blk.isValid()) {
                invalidate(&blk);
                invalidated++;
            }
        }

        DPRINTF(AdaptiveAssoc, "Cache flush completed. Invalidated %d blocks.\n",
                invalidated);
    }

    CacheBlk*
    AdaptiveAssoc::findVictim(const CacheBlk::KeyType& key,
                              const std::size_t size,
                              std::vector<CacheBlk*>& evict_blks,
                              const uint64_t partition_id)
    {
        std::vector<ReplaceableEntry*> entries = indexingPolicy->getPossibleEntries(key);
        std::vector<ReplaceableEntry*> filtered;
        
        for (auto* entry : entries) {
            CacheBlk* blk = static_cast<CacheBlk*>(entry);
            if (blk->getWay() < allocAssoc) {
                filtered.push_back(entry);
            }
        }
        CacheBlk* victim = filtered.empty() ? nullptr :
        static_cast<CacheBlk*>(replacementPolicy->getVictim(filtered));

        evict_blks.push_back(victim);
        return victim;
    }

} // namespace gem5
