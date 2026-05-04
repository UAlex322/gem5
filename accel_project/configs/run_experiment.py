import argparse

from m5.objects import (
    Cache,
    IOXBar,
    MatrixAccel,
)

from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.cachehierarchies.classic.no_cache import NoCache
from gem5.components.cachehierarchies.classic.private_l1_private_l2_cache_hierarchy import (
    PrivateL1PrivateL2CacheHierarchy,
)
from gem5.components.memory.single_channel import SingleChannelDDR4_2400
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.isas import ISA
from gem5.resources.resource import BinaryResource
from gem5.simulate.simulator import Simulator

parser = argparse.ArgumentParser(description="Matrix accelerator experiment")
parser.add_argument(
    "--cpu-type", choices=["timing", "o3"], default="timing", help="CPU model"
)
parser.add_argument(
    "--compute-latency",
    type=int,
    default=500,
    help="Accelerator compute latency in cycles",
)
parser.add_argument(
    "--binary", required=True, help="Path to the binary to run"
)
parser.add_argument(
    "--num-accels",
    type=int,
    choices=[0, 1, 2, 4],
    default=1,
    help="Number of accelerators (0 = CPU-only baseline)",
)
parser.add_argument(
    "--cache",
    action="store_true",
    help="Enable L1+L2 cache (32KiB L1D, 32KiB L1I, 256KiB L2)",
)
args = parser.parse_args()

cpu_type_map = {
    "timing": CPUTypes.TIMING,
    "o3": CPUTypes.O3,
}

if args.cache:
    cache_hierarchy = PrivateL1PrivateL2CacheHierarchy(
        l1d_size="32KiB",
        l1i_size="32KiB",
        l2_size="256KiB",
    )
else:
    cache_hierarchy = NoCache()
memory = SingleChannelDDR4_2400("512MiB")
processor = SimpleProcessor(
    cpu_type=cpu_type_map[args.cpu_type],
    isa=ISA.RISCV,
    num_cores=1,
)

board = SimpleBoard(
    clk_freq="1GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)

ACCEL_BASE = 0x50000000
ACCEL_STEP = 0x01000000

accels = []
for i in range(args.num_accels):
    accel = MatrixAccel(
        pio_addr=ACCEL_BASE + i * ACCEL_STEP,
        pio_size=0x400,
        compute_latency=args.compute_latency,
    )
    accel.pio = cache_hierarchy.membus.mem_side_ports
    setattr(board, f"matrix_accel_{i}", accel)
    accels.append(accel)

board.set_se_binary_workload(BinaryResource(local_path=args.binary))

# After set_se_binary_workload, board.mem_ranges is available.
# With cache: route DMA through IOXBar+IOCache so it generates snoop requests
# to invalidate Modified L1/L2 lines before writing (fixes cache.cc:1225 panic).
# Without cache: connect DMA directly to membus (no coherence issue).
if args.cache and accels:
    board.iobus = IOXBar()
    board.iocache = Cache(
        assoc=8,
        tag_latency=50,
        data_latency=50,
        response_latency=50,
        mshrs=20,
        size="1KiB",
        tgts_per_mshr=12,
        addr_ranges=board.mem_ranges,
    )
    board.iocache.mem_side = cache_hierarchy.membus.cpu_side_ports
    board.iocache.cpu_side = board.iobus.mem_side_ports
    for accel in accels:
        accel.dma = board.iobus.cpu_side_ports
else:
    for accel in accels:
        accel.dma = cache_hierarchy.membus.cpu_side_ports

print(
    f"--- cpu={args.cpu_type} cache={args.cache} compute_latency={args.compute_latency} "
    f"num_accels={args.num_accels} binary={args.binary} ---"
)

simulator = Simulator(board=board)
simulator.run()

print(f"--- Finished at tick {simulator.get_current_tick()} ---")
