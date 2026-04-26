import argparse

from m5.objects import MatrixAccel

from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.cachehierarchies.classic.no_cache import NoCache
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
    "--no-accel",
    action="store_true",
    help="Run without accelerator (CPU-only baseline)",
)
args = parser.parse_args()

cpu_type_map = {
    "timing": CPUTypes.TIMING,
    "o3": CPUTypes.O3,
}

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

if not args.no_accel:
    board.matrix_accel = MatrixAccel(
        pio_addr=0x50000000,
        pio_size=0x400,
        compute_latency=args.compute_latency,
    )
    board.matrix_accel.pio = cache_hierarchy.membus.mem_side_ports
    board.matrix_accel.dma = cache_hierarchy.membus.cpu_side_ports

board.set_se_binary_workload(BinaryResource(local_path=args.binary))

print(
    f"--- cpu={args.cpu_type} compute_latency={args.compute_latency} "
    f"accel={'no' if args.no_accel else 'yes'} binary={args.binary} ---"
)

simulator = Simulator(board=board)
simulator.run()

print(f"--- Finished at tick {simulator.get_current_tick()} ---")
