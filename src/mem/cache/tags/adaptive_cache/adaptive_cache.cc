#include "mem/cache/tags/adaptive_cache/adaptive_cache.hh"

namespace gem5
{

AdaptiveCache::AdaptiveCache(const AdaptiveCacheParams &p) : Cache(p)
{}

void
AdaptiveCache::init()
{
    Cache::init();

    AdaptiveAssoc *tmp = dynamic_cast<AdaptiveAssoc *>(tags);

    if (!tmp) {
        fatal("AdaptiveCache: tags is not AdaptiveAssoc!");
    }

    tmp->setCache(this);

    DPRINTF(AdaptiveAssoc, "AdaptiveCache connected\n");
}

} // namespace gem5
