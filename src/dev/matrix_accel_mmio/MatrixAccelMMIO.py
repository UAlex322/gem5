from m5.objects.Device import BasicPioDevice
from m5.params import *
from m5.proxy import *


class MatrixAccelMMIO(BasicPioDevice):
    type = "MatrixAccelMMIO"
    cxx_header = "dev/matrix_accel_mmio/matrix_accel_mmio.hh"
    cxx_class = "gem5::MatrixAccelMMIO"

    pio_size = Param.Addr(3 * 64 * 64 * 8 + 16, "Size of address range")
