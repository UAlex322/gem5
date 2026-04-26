import csv
import os

import matplotlib.pyplot as plt

CSV_FILE = "accel_project/results/csv/results_multi.csv"
PLOTS_DIR = "accel_project/results/plots"

os.makedirs(PLOTS_DIR, exist_ok=True)

rows = []
with open(CSV_FILE) as f:
    for row in csv.DictReader(f):
        row["simTicks"] = int(row["simTicks"])
        row["simInsts"] = int(row["simInsts"])
        rows.append(row)

num_accels = [int(r["num_accels"]) for r in rows]
ticks = [r["simTicks"] for r in rows]
baseline = ticks[0]
speedup = [baseline / t for t in ticks]

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))
fig.suptitle(
    "Multi-Accelerator Experiment (N=64, block=16, float, TIMING, latency=500)"
)

ax1.bar(
    [str(n) for n in num_accels], [t / 1e9 for t in ticks], color="steelblue"
)
ax1.set_xlabel("Number of accelerators")
ax1.set_ylabel("simTicks (x10^9)")
ax1.set_title("Simulation Time")
for i, (n, t) in enumerate(zip(num_accels, ticks)):
    ax1.text(i, t / 1e9 + 0.1, f"{t/1e9:.1f}", ha="center", fontsize=9)

ax2.plot([str(n) for n in num_accels], speedup, "o-", color="steelblue")
ax2.axhline(1.0, color="gray", linestyle="--", label="baseline (1 accel)")
ax2.plot(
    [str(n) for n in num_accels],
    [n for n in num_accels],
    "r--",
    label="ideal speedup",
)
ax2.set_xlabel("Number of accelerators")
ax2.set_ylabel("Speedup vs 1 accelerator")
ax2.set_title("Speedup")
ax2.legend()
ax2.grid(True)

plt.tight_layout()
plt.savefig(f"{PLOTS_DIR}/exp5_multi_accel.png", dpi=150)
print("Saved exp5_multi_accel.png")
