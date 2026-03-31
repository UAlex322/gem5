#include "mem/cache/tags/adaptive_cache/adaptive_assoc.hh"
#include "mem/cache/tags/indexing_policies/set_associative.hh"
#include "sim/system.hh"
#include "cpu/base.hh"

namespace gem5 {

    AdaptiveAssoc::AdaptiveAssoc(const AdaptiveAssocParams &p) : BaseSetAssoc(p),
        monitor(p.reconfig_period),
        current_assoc(p.assoc),
        target_assoc(p.assoc),
        need_reconfig(false),
        reconfig_in_progress(false),
        reconfig_cycles_left(0),
        cycle_counter(0),
        reconfig_count(0),
        reconfig_period(p.reconfig_period),
        reconfig_overhead(p.reconfig_overhead)
    {

        DPRINTF(AdaptiveAssoc, "Adaptive cache initialized.\n"
            "  Reconfig period: %d cycles\n"
            "  Reconfig overhead: %d cycles\n"
            "  Initial associativity: %d-way\n"
            "  Decision period: %d cycles (10%%)\n",
                reconfig_period, reconfig_overhead,
                current_assoc, reconfig_period / 10);
    }

    void AdaptiveAssoc::init() {
        BaseSetAssoc::init();

        // Set new start time

        monitor.startNewPeriod(curTick());

        // Get clock from processor

        if (system && !system->threads.empty() && system->threads[0])
            monitor.setCpuClock(system->threads[0]->getCpuPtr()->clockPeriod());
    }

    CacheBlk*
    AdaptiveAssoc::accessBlock(const PacketPtr pkt, Cycles &lat)
    {
        // Access from Parent

        CacheBlk* blk = BaseSetAssoc::accessBlock(pkt, lat);

        // Check hit or miss

        bool hit = (blk != nullptr && blk->isValid());
        monitor.onAccess(hit);

        // Update Cycle

        updateCycle(curTick());

        return blk;
    }

    void AdaptiveAssoc::updateCycle(Tick now)
    {
        cycle_counter++;

        // If reconfiguration

        if (reconfig_in_progress) {
            if (reconfig_cycles_left > 0) {
                reconfig_cycles_left--;
            }
            if (reconfig_cycles_left == 0) {
                reconfig_in_progress = false;
                need_reconfig = false;
                DPRINTF(AdaptiveAssoc, "Reconfiguration completed. "
                    "New associativity: %d-way\n", current_assoc);
            }
            return;
        }

        // Check if Decision Time

        if (monitor.isDecisionTime(now)) {
            monitor.endDecisionPhase(now, current_assoc);

            CacheFeatures features;
            if (monitor.getFeatures(features)) {
                int predicted = decisionTree.predict(features);
                target_assoc = static_cast<unsigned>(predicted);

                DPRINTF(AdaptiveAssoc, "Decision: miss_rate_pct=%.3f%%, "
                    "miss_count=%llu, total_mem=%llu, ipc=%.3f, prev_assoc=%llu -> %d-way\n",
                    features.miss_rate_pct,
                    (unsigned long long)features.miss_count,
                    (unsigned long long)features.total_mem_access,
                    features.ipc,
                    (unsigned long long)features.prev_assoc,
                    target_assoc);

                if (target_assoc != current_assoc) {
                    need_reconfig = true;
                    DPRINTF(AdaptiveAssoc, "Scheduling reconfiguration: "
                        "%d-way -> %d-way\n", current_assoc, target_assoc);
                }
            }
        }

        // Is period end

        if (monitor.isPeriodEnd(now)) {
            monitor.startNewPeriod(now);
            if (need_reconfig) {
                reconfigureAssociativity(target_assoc);
            }
        }
    }

    void AdaptiveAssoc::reconfigureAssociativity(unsigned new_assoc)
    {
        if (new_assoc == current_assoc) return;

        DPRINTF(AdaptiveAssoc, "Starting reconfiguration: %d-way -> %d-way\n",
                current_assoc, new_assoc);

        // Writeback to mem

        writebackDirtyBlocks();

        // Cleat cache

        flushCache();

        // Set max way allocation

        setWayAllocationMax(new_assoc);

        // Change associativity

        current_assoc = new_assoc;

        // Reconfiguration overhead

        reconfig_cycles_left = reconfig_overhead;
        reconfig_in_progress = true;
        reconfig_count++;

        DPRINTF(AdaptiveAssoc, "Reconfiguration started. %d cycles remaining\n",
                reconfig_cycles_left);
    }

    void AdaptiveAssoc::writebackDirtyBlocks()
    {
        int dirty_count = 0;

        // Find dirty blocks

        for (auto& blk : blks) {
            if (blk.isSet(CacheBlk::DirtyBit)) {

                // Restore physical addr

                Addr blk_addr = regenerateBlkAddr(&blk);

                DPRINTF(AdaptiveAssoc, "Writeback: addr=%#lx\n", blk_addr);

                // Make request

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

        // Invalidate all

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
        // Get all entries from indexing policy

        std::vector<ReplaceableEntry*> entries =
        indexingPolicy->getPossibleEntries(key);

        // Just if way more than allocAssoc

        std::vector<ReplaceableEntry*> filtered;
        for (auto* entry : entries) {
            CacheBlk* blk = static_cast<CacheBlk*>(entry);
            if (blk->getWay() < allocAssoc) {
                filtered.push_back(entry);
            }
        }

        // Get filtered victim

        CacheBlk* victim = filtered.empty() ? nullptr :
        static_cast<CacheBlk*>(replacementPolicy->getVictim(filtered));

        evict_blks.push_back(victim);
        return victim;
    }

} // namespace gem5
