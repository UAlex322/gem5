from m5.objects import BaseSetAssoc
from m5.params import *

class AdaptiveAssoc(BaseSetAssoc):
    type = "AdaptiveAssoc"
    cxx_header = "mem/cache/tags/adaptive_cache/adaptive_assoc.hh"
    cxx_class = "gem5::AdaptiveAssoc"

    # Get time of reconfiguration period

    reconfig_period = Param.Int(30000000, "reconfiguration period")

    # Reconfiguration delay

    reconfig_overhead = Param.Int(500, "Reconfiguration overhead in cycles (500 from paper)")
