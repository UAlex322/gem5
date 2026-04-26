import csv
import os

import matplotlib.pyplot as plt

CSV_FILE = "accel_project/results/csv/results.csv"
PLOTS_DIR = "accel_project/results/plots"

os.makedirs(PLOTS_DIR, exist_ok=True)

# load CSV
rows = []
with open(CSV_FILE) as f:
    for row in csv.DictReader(f):
        row["simTicks"] = int(row["simTicks"])
        row["simInsts"] = int(row["simInsts"])
        rows.append(row)


def get(mode, cpu, block, dtype, latency=None):
    for r in rows:
        if (
            r["mode"] == mode
            and r["cpu_type"] == cpu
            and r["block_size"] == str(block)
            and r["data_type"] == dtype
        ):
            if latency is None or r["compute_latency"] == str(latency):
                return r
    return None


# ── Plot 1: accel vs CPU, block_size sweep ────────────────────
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))
fig.suptitle(
    "Experiment 1: Accelerator vs CPU  (N=64, float, TIMING, latency=500)"
)

blocks = [16, 32, 64]
accel_ticks = [
    get("accel", "timing", b, "float", 500)["simTicks"] for b in blocks
]
cpu_ticks = [
    get("cpu", "timing", b, "float", None)["simTicks"] for b in blocks
]
accel_insts = [
    get("accel", "timing", b, "float", 500)["simInsts"] for b in blocks
]
cpu_insts = [
    get("cpu", "timing", b, "float", None)["simInsts"] for b in blocks
]

x = range(len(blocks))
w = 0.35
ax1.bar(
    [i - w / 2 for i in x], [t / 1e9 for t in accel_ticks], w, label="Accel"
)
ax1.bar([i + w / 2 for i in x], [t / 1e9 for t in cpu_ticks], w, label="CPU")
ax1.set_xticks(list(x))
ax1.set_xticklabels([f"block={b}" for b in blocks])
ax1.set_ylabel("simTicks (×10⁹)")
ax1.set_title("Simulation Time")
ax1.legend()

ax2.bar(
    [i - w / 2 for i in x], [t / 1e6 for t in accel_insts], w, label="Accel"
)
ax2.bar([i + w / 2 for i in x], [t / 1e6 for t in cpu_insts], w, label="CPU")
ax2.set_xticks(list(x))
ax2.set_xticklabels([f"block={b}" for b in blocks])
ax2.set_ylabel("simInsts (×10⁶)")
ax2.set_title("Instructions Executed")
ax2.legend()

plt.tight_layout()
plt.savefig(f"{PLOTS_DIR}/exp1_blocksize_sweep.png", dpi=150)
print("Saved exp1_blocksize_sweep.png")

# ── Plot 2: compute_latency sweep ─────────────────────────────
fig, ax = plt.subplots(figsize=(8, 5))
fig.suptitle("Experiment 3: Latency Sweep  (N=64, block=64, float, TIMING)")

latencies = [100, 500, 2000, 10000]
accel_ticks = [
    get("accel", "timing", 64, "float", lat)["simTicks"] for lat in latencies
]
cpu_ref = get("cpu", "timing", 64, "float")["simTicks"]

ax.plot(latencies, [t / 1e9 for t in accel_ticks], "o-", label="Accel")
ax.axhline(cpu_ref / 1e9, color="red", linestyle="--", label="CPU baseline")
ax.set_xlabel("compute_latency (cycles)")
ax.set_ylabel("simTicks (×10⁹)")
ax.set_title("Accelerator vs CPU at different compute latencies")
ax.legend()
ax.grid(True)

plt.tight_layout()
plt.savefig(f"{PLOTS_DIR}/exp3_latency_sweep.png", dpi=150)
print("Saved exp3_latency_sweep.png")

# ── Plot 2: TIMING vs O3 ──────────────────────────────────────
fig, ax = plt.subplots(figsize=(8, 5))
fig.suptitle("Experiment 2: CPU Type  (N=64, block=64, float, latency=500)")

labels = ["Accel TIMING", "Accel O3", "CPU TIMING", "CPU O3"]
ticks = [
    get("accel", "timing", 64, "float", 500)["simTicks"],
    get("accel", "o3", 64, "float", 500)["simTicks"],
    get("cpu", "timing", 64, "float")["simTicks"],
    get("cpu", "o3", 64, "float")["simTicks"],
]
colors = ["steelblue", "steelblue", "orange", "orange"]
bars = ax.bar(labels, [t / 1e9 for t in ticks], color=colors)
ax.set_ylabel("simTicks (×10⁹)")
ax.set_title("TIMING vs O3: Accelerator and CPU")
for bar, t in zip(bars, ticks):
    ax.text(
        bar.get_x() + bar.get_width() / 2,
        bar.get_height() + 0.5,
        f"{t/1e9:.1f}",
        ha="center",
        va="bottom",
        fontsize=9,
    )

plt.tight_layout()
plt.savefig(f"{PLOTS_DIR}/exp2_cpu_type.png", dpi=150)
print("Saved exp2_cpu_type.png")

# ── Plot 3: data type comparison ──────────────────────────────
fig, ax = plt.subplots(figsize=(8, 5))
fig.suptitle("Experiment 4: Data Type  (N=64, block=32, TIMING, latency=500)")

dtypes = ["int", "float", "double"]
accel_ticks = [get("accel", "timing", 32, d, 500)["simTicks"] for d in dtypes]
cpu_ticks = [get("cpu", "timing", 32, d, None)["simTicks"] for d in dtypes]

x = range(len(dtypes))
ax.bar(
    [i - w / 2 for i in x], [t / 1e9 for t in accel_ticks], w, label="Accel"
)
ax.bar([i + w / 2 for i in x], [t / 1e9 for t in cpu_ticks], w, label="CPU")
ax.set_xticks(list(x))
ax.set_xticklabels(dtypes)
ax.set_ylabel("simTicks (×10⁹)")
ax.set_title("Simulation Time by Data Type")
ax.legend()

plt.tight_layout()
plt.savefig(f"{PLOTS_DIR}/exp4_datatype.png", dpi=150)
print("Saved exp4_datatype.png")
