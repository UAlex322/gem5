from m5.objects.Device import BasicPioDevice
from m5.params import *
from m5.proxy import *


class MatrixAccel(BasicPioDevice):
    type = "MatrixAccel"
    cxx_header = "dev/matrix_accel/matrix_accel.hh"
    cxx_class = "gem5::MatrixAccel"

    mem_port = RequestPort("DMA port")
    pio_size = Param.Addr(1024, "Size of address range")
