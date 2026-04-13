#ifndef __MEM_CACHE_INDEXING_POLICIES_ADAPTIVE_INDEX_HH__
#define __MEM_CACHE_INDEXING_POLICIES_ADAPTIVE_INDEX_HH__

#include <vector>

#include "mem/cache/tags/tagged_entry.hh"
#include "params/AdaptiveIndex.hh"

namespace gem5
{

class ReplaceableEntry;
class AdaptiveIndex : public TaggedIndexingPolicy
{
  protected:
    const uint64_t size;
    const unsigned blockSize;
    const uint64_t numEntries;

    unsigned currentAssoc;
    unsigned currentNumSets;
    int currentSetShift;
    unsigned currentSetMask;
    int currentTagShift;

    uint32_t extractSet(const KeyType &key) const;
    void rebuildStructure(unsigned new_assoc);

  public:
    AdaptiveIndex(const AdaptiveIndexParams &p);
    ~AdaptiveIndex() {};

    void setEntry(ReplaceableEntry *entry, const uint64_t index) override;
    void setAssociativity(unsigned new_assoc);
    std::vector<ReplaceableEntry *>
    getPossibleEntries(const KeyType &key) const override;

    Addr regenerateAddr(const KeyType &key,
                        const ReplaceableEntry *entry) const override;
    Addr extractTag(const Addr addr) const override;

    unsigned
    getCurrentAssoc() const
    {
        return currentAssoc;
    }
    unsigned
    getCurrentNumSets() const
    {
        return currentNumSets;
    }
};

} // namespace gem5

#endif //__MEM_CACHE_INDEXING_POLICIES_ADAPTIVE_INDEX_HH__
