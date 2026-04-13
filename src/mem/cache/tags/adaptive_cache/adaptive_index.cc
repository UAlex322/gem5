#include "mem/cache/tags/adaptive_cache/adaptive_index.hh"

#include "base/intmath.hh"

namespace gem5
{

AdaptiveIndex::AdaptiveIndex(const AdaptiveIndexParams &p)
    : TaggedIndexingPolicy(p, 0, 0),
      size(p.size),
      blockSize(p.block_size),
      numEntries(size / blockSize),
      currentAssoc(p.assoc)
{
    rebuildStructure(currentAssoc);
}

void
AdaptiveIndex::rebuildStructure(unsigned new_assoc)
{
    currentAssoc = new_assoc;
    currentNumSets = numEntries / currentAssoc;

    fatal_if(!isPowerOf2(currentNumSets),
             "Number of sets (%d) must be a power of 2", currentNumSets);

    currentSetShift = floorLog2(blockSize);
    currentSetMask = currentNumSets - 1;
    currentTagShift = currentSetShift + floorLog2(currentNumSets);

    auto old_sets = std::move(sets);

    sets.clear();
    sets.resize(currentNumSets);
    for (uint32_t i = 0; i < currentNumSets; ++i) {
        sets[i].resize(currentAssoc, nullptr);
    }

    if (!old_sets.empty()) {
        unsigned old_num_sets = old_sets.size();
        unsigned old_assoc = old_sets[0].size();

        for (uint32_t old_set = 0; old_set < old_num_sets; ++old_set) {
            for (uint32_t old_way = 0; old_way < old_assoc; ++old_way) {
                ReplaceableEntry *entry = old_sets[old_set][old_way];
                if (entry) {
                    uint32_t new_set = old_set % currentNumSets;
                    uint32_t new_way = 0;

                    while (new_way < currentAssoc && sets[new_set][new_way]) {
                        new_way++;
                    }

                    if (new_way < currentAssoc) {
                        sets[new_set][new_way] = entry;
                        entry->setPosition(new_set, new_way);
                    }
                }
            }
        }
    }
}

void
AdaptiveIndex::setAssociativity(unsigned new_assoc)
{
    if (new_assoc == currentAssoc) {
        return;
    }
    rebuildStructure(new_assoc);
}

uint32_t
AdaptiveIndex::extractSet(const KeyType &key) const
{
    return (key.address >> currentSetShift) & currentSetMask;
}

std::vector<ReplaceableEntry *>
AdaptiveIndex::getPossibleEntries(const KeyType &key) const
{
    return sets[extractSet(key)];
}

Addr
AdaptiveIndex::regenerateAddr(const KeyType &key,
                              const ReplaceableEntry *entry) const
{
    return (key.address << currentTagShift) |
           (entry->getSet() << currentSetShift);
}

Addr
AdaptiveIndex::extractTag(const Addr addr) const
{
    return addr >> currentTagShift;
}

void
AdaptiveIndex::setEntry(ReplaceableEntry *entry, const uint64_t index)
{
    const std::lldiv_t div_result = std::div((long long)index, currentAssoc);
    const uint32_t set = div_result.quot;
    const uint32_t way = div_result.rem;

    assert(set < currentNumSets);
    sets[set][way] = entry;
    entry->setPosition(set, way);
}

} // namespace gem5
