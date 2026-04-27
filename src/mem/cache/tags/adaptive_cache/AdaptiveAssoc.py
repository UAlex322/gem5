from m5.objects import (
    BaseCPU,
    BaseSetAssoc,
    Cache,
    TaggedIndexingPolicy,
)
from m5.params import *
from m5.proxy import *


class AdaptiveIndex(TaggedIndexingPolicy):
    type = "AdaptiveIndex"
    cxx_header = "mem/cache/tags/adaptive_cache/adaptive_index.hh"
    cxx_class = "gem5::AdaptiveIndex"

    size = Param.MemorySize(Parent.size, "Cache size")
    block_size = Param.Int(Parent.block_size, "Block size")


class AdaptiveAssoc(BaseSetAssoc):
    type = "AdaptiveAssoc"
    cxx_header = "mem/cache/tags/adaptive_cache/adaptive_assoc.hh"
    cxx_class = "gem5::AdaptiveAssoc"

    # Get time of reconfiguration period
    reconfig_period = Param.Int(30000000, "reconfiguration period")

    # Get cores
    cpus = VectorParam.BaseCPU([], "std::vector of cpu cores")


class AdaptiveCache(Cache):
    type = "AdaptiveCache"
    cxx_header = "mem/cache/tags/adaptive_cache/adaptive_cache.hh"
    cxx_class = "gem5::AdaptiveCache"
