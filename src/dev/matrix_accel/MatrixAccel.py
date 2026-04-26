from m5.objects.Device import DmaVirtDevice
from m5.params import *
from m5.proxy import *


class MatrixAccel(DmaVirtDevice):
    type = "MatrixAccel"
    cxx_header = "dev/matrix_accel/matrix_accel.hh"
    cxx_class = "gem5::MatrixAccel"

    pio_addr = Param.Addr("Device Address")
    pio_size = Param.Addr(1024, "Size of address range")
    pio_latency = Param.Latency("100ns", "Programmed IO latency")
    compute_latency = Param.Cycles(500, "Compute latency in cycles")
