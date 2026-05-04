from m5.objects import MatrixAccel

from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.cachehierarchies.classic.no_cache import NoCache
from gem5.components.memory.single_channel import SingleChannelDDR4_2400
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.isas import ISA
from gem5.resources.resource import BinaryResource
from gem5.simulate.simulator import Simulator

cache_hierarchy = NoCache()
memory = SingleChannelDDR4_2400("512MiB")
processor = SimpleProcessor(
    cpu_type=CPUTypes.TIMING, isa=ISA.RISCV, num_cores=1
)

board = SimpleBoard(
    clk_freq="1GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)

board.matrix_accel = MatrixAccel(pio_addr=0x50000000, pio_size=0x400)

board.matrix_accel.pio = cache_hierarchy.membus.mem_side_ports
board.matrix_accel.dma = cache_hierarchy.membus.cpu_side_ports

binary = BinaryResource(local_path="accel_project/bin/simple_mult_test_O2")
board.set_se_binary_workload(binary)

simulator = Simulator(board=board)
print("--- Starting Modern Gem5 Simulation ---")
simulator.run()
print(f"--- Finished at tick {simulator.get_current_tick()} ---")
