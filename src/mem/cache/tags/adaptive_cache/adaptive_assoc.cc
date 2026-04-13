#include "mem/cache/tags/adaptive_cache/adaptive_assoc.hh"

#include "cpu/base.hh"
#include "mem/cache/tags/indexing_policies/set_associative.hh"
#include "sim/system.hh"

namespace gem5
{

AdaptiveAssoc::CacheFeatures::CacheFeatures(double _miss_rate,
                                            uint64_t _miss_count,
                                            uint64_t _total_memory_access,
                                            double _ipc, unsigned _prev_assoc)
    : miss_rate(_miss_rate),
      miss_count(_miss_count),
      total_mem_access(_total_memory_access),
      ipc(_ipc),
      prev_assoc(_prev_assoc)
{}

unsigned
AdaptiveAssoc::DecisionTree::Predict(const CacheFeatures &f) const
{
    if (f.miss_rate < 0.27) {
        switch (f.prev_assoc) {
            case 1:
                return 1;
            case 2:
                return 2;
            case 4:
                return 4;
            case 8:
                return 8;
            case 16:
                return 16;
            default:
                return 16;
        }
    } else if (f.miss_rate < 0.48) {
        if (f.total_mem_access < 5833) {
            return 2;
        } else if (f.total_mem_access < 9241) {
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
        } else {
            if (f.miss_count < 19441) {
                return 4;
            } else if (f.miss_count < 603810) {
                return 8;
            } else {
                return 16;
            }
        }
    } else if (f.miss_rate <= 2.52) {
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

uint64_t
AdaptiveAssoc::PerformanceMonitor::executedInsts()
{
    uint64_t tmp_insts = 0;
    for (size_t i = 0; i < cache_tag->cpus.size(); i++) {
        for (ThreadID j = 0; j < cache_tag->cpus[i]->numThreads; j++) {
            tmp_insts += cache_tag->cpus[i]->getCurrentInstCount(j);
        }
    }
    return tmp_insts;
}

void
AdaptiveAssoc::PerformanceMonitor::processNextDecisionEndEvent()
{
    instructions = executedInsts() - instructions;
    uint64_t cycles = (curTick() - period_start) / proc_time_clock;

    if (mem_accesses != 0) {
        features.miss_rate = (double)cache_misses / mem_accesses * 100;
    } else {
        return;
    }
    if (cycles != 0) {
        features.ipc = (double)instructions / cycles;
    } else {
        return;
    }
    features.miss_count = cache_misses;
    features.total_mem_access = mem_accesses;
    features.prev_assoc = cache_tag->current_assoc;

    unsigned new_assoc = cache_tag->decision_tree.Predict(features);
    if (new_assoc == cache_tag->current_assoc) {
        return;
    }
    cache_tag->reconfigureAssociativity(new_assoc);
}

void
AdaptiveAssoc::PerformanceMonitor::processPeriodEndEvent()
{
    startNewPeriod(curTick());
}

AdaptiveAssoc::PerformanceMonitor::PerformanceMonitor(
    AdaptiveAssoc *_cache_tag, uint64_t _reconfig_period,
    Tick _proc_time_clock)
    : instructions(0),
      mem_accesses(0),
      cache_misses(0),
      cache_tag(_cache_tag),
      proc_time_clock(_proc_time_clock),
      period_start(0),
      reconfig_period(_reconfig_period * _proc_time_clock),
      decision_period(_reconfig_period * _proc_time_clock / 10)
{}

void
AdaptiveAssoc::PerformanceMonitor::setCpuClock(Tick tm)
{
    proc_time_clock = tm;
}

void
AdaptiveAssoc::PerformanceMonitor::onAccess(bool hit)
{
    mem_accesses++;
    if (!hit) {
        cache_misses++;
    }
}

void
AdaptiveAssoc::PerformanceMonitor::startNewPeriod(Tick now)
{
    period_start = now;
    instructions = executedInsts();
    mem_accesses = 0;
    cache_misses = 0;

    if (cache_tag->nextDecisionEndEvent.scheduled()) {
        cache_tag->deschedule(cache_tag->nextDecisionEndEvent);
    }
    cache_tag->schedule(cache_tag->nextDecisionEndEvent,
                        now + decision_period);

    if (cache_tag->nextPeriodEndEvent.scheduled()) {
        cache_tag->deschedule(cache_tag->nextPeriodEndEvent);
    }
    cache_tag->schedule(cache_tag->nextPeriodEndEvent, now + reconfig_period);
}

AdaptiveAssoc::AdaptiveAssoc(const AdaptiveAssocParams &p)
    : BaseSetAssoc(p),
      monitor(this, p.reconfig_period),
      parent_cache(p.parent_cache),
      cpus(p.cpus),
      current_assoc(16),
      reconfig_period(p.reconfig_period),
      nextDecisionEndEvent([this]() { monitor.processNextDecisionEndEvent(); },
                           name() + "nextDecisionEndEvent"),
      nextPeriodEndEvent([this]() { monitor.processPeriodEndEvent(); },
                         name() + "nextPeriodEndEvent")
{
    DPRINTF(AdaptiveAssoc,
            "Adaptive cache initialized.\n"
            "  Reconfig period: %d cycles\n"
            "  Initial associativity: %d-way\n"
            "  Decision period: %d cycles (10%%)\n",
            reconfig_period, current_assoc, reconfig_period / 10);
}

void
AdaptiveAssoc::init()
{
    BaseSetAssoc::init();
    if (cpus.size() != 0) {
        BaseCPU *cpu = cpus[0];
        monitor.setCpuClock(cpu->clockPeriod());
        monitor.startNewPeriod(cpu->clockEdge());
    }
    adaptive_index = static_cast<AdaptiveIndex *>(indexingPolicy);
}

CacheBlk *
AdaptiveAssoc::accessBlock(const PacketPtr pkt, Cycles &lat)
{
    CacheBlk *blk = BaseSetAssoc::accessBlock(pkt, lat);
    bool hit = (blk != nullptr && blk->isValid());
    monitor.onAccess(hit);
    return blk;
}

void
AdaptiveAssoc::reconfigureAssociativity(unsigned new_assoc)
{
    DPRINTF(AdaptiveAssoc, "Starting reconfiguration: %d-way -> %d-way\n",
            current_assoc, new_assoc);
    writebackDirtyBlocks();
    flushCache();
    setWayAllocationMax(new_assoc);
    adaptive_index->setAssociativity(new_assoc);
    current_assoc = new_assoc;
}

unsigned
AdaptiveAssoc::getCurrentAssoc() const
{
    return current_assoc;
}

uint64_t
AdaptiveAssoc::getReconfigPeriod() const
{
    return reconfig_period;
}

void
AdaptiveAssoc::writebackDirtyBlocks()
{
    for (auto &blk : blks) {
        if (blk.isSet(CacheBlk::DirtyBit)) {
            PacketPtr pkt = parent_cache->writebackBlk(&blk);
            if (pkt) {
                DPRINTF(AdaptiveAssoc,
                        "Timing writeback initiated for addr=%#lx\n",
                        regenerateBlkAddr(&blk));
            } else {
                DPRINTF(AdaptiveAssoc,
                        "Failed to initiate writeback for addr=%#lx\n",
                        regenerateBlkAddr(&blk));
            }
        }
    }
}

void
AdaptiveAssoc::flushCache()
{
    int invalidated = 0;
    for (auto &blk : blks) {
        if (blk.isValid()) {
            invalidate(&blk);
            invalidated++;
        }
    }

    DPRINTF(AdaptiveAssoc, "Cache flush completed. %d blocks invalidated.\n",
            invalidated);
}

} // namespace gem5
