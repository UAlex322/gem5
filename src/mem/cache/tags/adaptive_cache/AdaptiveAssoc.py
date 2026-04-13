from m5.objects import (
    BaseCache,
    BaseCPU,
    BaseSetAssoc,
    TaggedIndexingPolicy,
)
from m5.params import *
from m5.proxy import *


class AdaptiveIndex(TaggedIndexingPolicy):
    type = "AdaptiveIndex"
    cxx_header = "mem/cache/tags/adaptive_cache/adaptive_index.hh"
    cxx_class = "gem5::AdaptiveIndex"

    size = Param.MemorySize("Cache size")
    assoc = Param.Unsigned(16, "Initial associativity")
    block_size = Param.Unsigned(64, "Block size")


class AdaptiveAssoc(BaseSetAssoc):
    type = "AdaptiveAssoc"
    cxx_header = "mem/cache/tags/adaptive_cache/adaptive_assoc.hh"
    cxx_class = "gem5::AdaptiveAssoc"

    # Get time of reconfiguration period
    reconfig_period = Param.Int(30000000, "reconfiguration period")

    # Get parent cache
    parent_cache = Param.BaseCache(Parent.any, "pointer to cache")

    # Get cores
    cpus = VectorParam.BaseCPU([], "std::vector of cpu cores")

    # Get indexing policy
    indexing_policy = Param.TaggedIndexingPolicy(
        AdaptiveIndex(), "Indexing policy"
    )
