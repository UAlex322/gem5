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
AdaptiveAssoc::DecisionTree::Predict(const CacheFeatures &f)
{
    switch (test_var) {
        case 0:
            test_var = 1;
            return 8;
        case 1:
            test_var = 2;
            return 4;
        case 2:
            test_var = 3;
            return 2;
        case 3:
            test_var = 4;
            return 1;
        case 4:
            test_var = 0;
            return 16;
        default:
            break;
    }
    /*
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
    }*/
    return 16;
}

uint64_t
AdaptiveAssoc::PerformanceMonitor::executedInsts()
{
    uint64_t tmp_insts = 0;
    for (size_t i = 0; i < cache_tag->cpus.size(); ++i) {
        for (ThreadID j = 0; j < cache_tag->cpus[i]->numThreads; ++j) {
            tmp_insts += cache_tag->cpus[i]->getCurrentInstCount(j);
        }
    }
    return tmp_insts;
}

void
AdaptiveAssoc::PerformanceMonitor::processNextDecisionEndEvent()
{
    cache_tag->toDPRINTF("PerformanceMonitor::processNextDecisionEndEvent()");
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
    cache_tag->toDPRINTF("PerformanceMonitor::startNewPeriod()");
}

AdaptiveAssoc::AdaptiveAssoc(const AdaptiveAssocParams &p)
    : BaseSetAssoc(p),
      monitor(this, p.reconfig_period),
      parent_cache(nullptr),
      cpus(p.cpus),
      current_assoc(16),
      reconfig_period(p.reconfig_period),
      nextDecisionEndEvent([this]() { monitor.processNextDecisionEndEvent(); },
                           name() + "nextDecisionEndEvent"),
      nextPeriodEndEvent([this]() { monitor.processPeriodEndEvent(); },
                         name() + "nextPeriodEndEvent"),
      adaptive_index(nullptr)
{}

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
    toDPRINTF("AdaptiveAssoc::init()");
}

CacheBlk *
AdaptiveAssoc::accessBlock(const PacketPtr pkt, Cycles &lat)
{
    DPRINTF(AdaptiveOthers, "AdaptiveAssoc::accessBlock()\n");
    CacheBlk *blk = BaseSetAssoc::accessBlock(pkt, lat);
    bool hit = (blk != nullptr && blk->isValid());
    monitor.onAccess(hit);
    return blk;
}

void
AdaptiveAssoc::reconfigureAssociativity(unsigned new_assoc)
{
    DPRINTF(AdaptiveAssoc,
            "AdaptiveAssoc::reconfigureAssociativity(\n"
            "\tcurrent_assoc: %d\n"
            "\tnew_assoc: %d\n)\n",
            current_assoc, new_assoc);
    parent_cache->cpuSidePort.setBlocked();
    writebackDirtyBlocks();
    // parent_cache->memWriteback();
    flushCache();
    setWayAllocationMax(new_assoc);
    adaptive_index->setAssociativity(new_assoc);
    /*for (unsigned i = 0; i < numBlocks; i++) {
        indexingPolicy->setEntry(&blks[i], i);
    }*/
    tagsInit();
    current_assoc = new_assoc;
    parent_cache->cpuSidePort.clearBlocked();
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
    PacketList writebacks;

    for (auto &blk : blks) {
        if (blk.isSet(CacheBlk::DirtyBit)) {
            PacketPtr pkt = parent_cache->writebackBlk(&blk);
            if (pkt) {
                writebacks.push_back(pkt);
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
    if (!writebacks.empty()) {
        DPRINTF(AdaptiveAssoc, "Submitted %d writebacks to doWritebacks\n",
                writebacks.size());
        parent_cache->doWritebacks(writebacks, curTick());
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

    DPRINTF(AdaptiveAssoc,
            "AdaptiveAssoc::flushCache()\n"
            "\tinvalidated: %d\n",
            invalidated);
}

void
AdaptiveAssoc::toDPRINTF(const char *msg)
{
    DPRINTF(AdaptiveAssoc,
            "%s\n"
            "\tAdaptiveAssoc.State =\n"
            "\t\tcurrent_assoc: %d\n"
            "\t\treconfig_period: %d\n"
            "\tPerformanceMonitor.State =\n"
            "\t\tinstructions: %d\n"
            "\t\tmem_accesses: %d\n"
            "\t\tcache_misses: %d\n"
            "\t\tproc_time_clock: %d\n"
            "\t\tperiod_start: %d\n"
            "\t\treconfig_period: %d\n"
            "\t\tdecision_period: %d\n",
            msg, current_assoc, reconfig_period, monitor.instructions,
            monitor.mem_accesses, monitor.cache_misses,
            monitor.proc_time_clock, monitor.period_start,
            monitor.reconfig_period, monitor.decision_period);
}

} // namespace gem5
