#ifndef __MEM_CACHE_TAGS_ADAPTIVE_CACHE_ADAPTIVE_CACHE_HH__
#define __MEM_CACHE_TAGS_ADAPTIVE_CACHE_ADAPTIVE_CACHE_HH__

#include "mem/cache/cache.hh"
#include "mem/cache/tags/adaptive_cache/adaptive_assoc.hh"
#include "params/AdaptiveCache.hh"

namespace gem5
{

class AdaptiveCache : public Cache
{
  public:
    AdaptiveCache(const AdaptiveCacheParams &p);
    void init() override;
};

} // namespace gem5

#endif //__MEM_CACHE_TAGS_ADAPTIVE_CACHE_ADAPTIVE_CACHE_HH__
