import csv
import os

import matplotlib.pyplot as plt
import numpy as np

CSV_FILE = "accel_project/results/csv/results_cache.csv"
PLOTS_DIR = "accel_project/results/plots"

os.makedirs(PLOTS_DIR, exist_ok=True)

rows = []
with open(CSV_FILE) as f:
    for row in csv.DictReader(f):
        row["simTicks"] = int(row["simTicks"])
        row["simInsts"] = int(row["simInsts"])
        rows.append(row)


def get(mode, cpu, cache):
    for r in rows:
        if r["mode"] == mode and r["cpu_type"] == cpu and r["cache"] == cache:
            return r
    return None


# ── Plot 1: grouped bars — no cache vs cache for all 4 configs ───────────────
fig, ax = plt.subplots(figsize=(11, 6))
fig.suptitle(
    "Experiment 6: CPU vs Accelerator with L1+L2 Cache\n"
    "(N=64, block=64, float, compute_latency=500)",
    fontsize=12,
)

configs = [
    ("cpu", "timing", "CPU\nTIMING"),
    ("cpu", "o3", "CPU\nO3"),
    ("accel", "timing", "Accel\nTIMING"),
    ("accel", "o3", "Accel\nO3"),
]

x = np.arange(len(configs))
w = 0.35

no_cache_ticks = [get(m, c, "no")["simTicks"] for m, c, _ in configs]
cache_ticks = [get(m, c, "yes")["simTicks"] for m, c, _ in configs]

bars1 = ax.bar(
    x - w / 2,
    [t / 1e9 for t in no_cache_ticks],
    w,
    label="No cache",
    color="steelblue",
)
bars2 = ax.bar(
    x + w / 2,
    [t / 1e9 for t in cache_ticks],
    w,
    label="L1+L2 cache",
    color="darkorange",
)

# value labels on bars
for bar, t in zip(bars1, no_cache_ticks):
    ax.text(
        bar.get_x() + bar.get_width() / 2,
        bar.get_height() + 0.5,
        f"{t/1e9:.1f}",
        ha="center",
        va="bottom",
        fontsize=8,
    )
for bar, t in zip(bars2, cache_ticks):
    val = t / 1e9
    label = f"{val:.3f}" if val < 1 else f"{val:.1f}"
    ax.text(
        bar.get_x() + bar.get_width() / 2,
        bar.get_height() + 0.5,
        label,
        ha="center",
        va="bottom",
        fontsize=8,
    )

ax.set_xticks(x)
ax.set_xticklabels([label for _, _, label in configs])
ax.set_ylabel("simTicks (×10⁹)")
ax.set_title("Simulation Time: No Cache vs L1+L2 Cache")
ax.legend()
ax.grid(axis="y", alpha=0.3)

plt.tight_layout()
plt.savefig(f"{PLOTS_DIR}/exp6_cache_comparison.png", dpi=150)
print("Saved exp6_cache_comparison.png")

# ── Plot 2: two panels (CPU left, Accel right) — linear scale, both readable ──
fig, (ax_cpu, ax_accel) = plt.subplots(1, 2, figsize=(13, 6))
fig.suptitle(
    "Experiment 6: Cache Effect — CPU (left) vs Accelerator (right)\n"
    "(N=64, block=64, float)",
    fontsize=12,
)

cpu_configs = [("cpu", "timing", "CPU\nTIMING"), ("cpu", "o3", "CPU\nO3")]
accel_configs = [
    ("accel", "timing", "Accel\nTIMING"),
    ("accel", "o3", "Accel\nO3"),
]

for ax, panel_configs, title in [
    (ax_cpu, cpu_configs, "CPU (no accelerator)"),
    (ax_accel, accel_configs, "Accelerator"),
]:
    px = np.arange(len(panel_configs))
    nc_ticks = [get(m, c, "no")["simTicks"] for m, c, _ in panel_configs]
    ca_ticks = [get(m, c, "yes")["simTicks"] for m, c, _ in panel_configs]

    b1 = ax.bar(
        px - w / 2,
        [t / 1e9 for t in nc_ticks],
        w,
        label="No cache",
        color="steelblue",
    )
    b2 = ax.bar(
        px + w / 2,
        [t / 1e9 for t in ca_ticks],
        w,
        label="L1+L2 cache",
        color="darkorange",
    )

    for bar, t in zip(b1, nc_ticks):
        ax.text(
            bar.get_x() + bar.get_width() / 2,
            bar.get_height() + 0.02 * max(nc_ticks) / 1e9,
            f"{t/1e9:.1f}",
            ha="center",
            va="bottom",
            fontsize=9,
        )
    for bar, t in zip(b2, ca_ticks):
        val = t / 1e9
        label = f"{val:.3f}" if val < 1 else f"{val:.2f}"
        ax.text(
            bar.get_x() + bar.get_width() / 2,
            bar.get_height() + 0.02 * max(nc_ticks) / 1e9,
            label,
            ha="center",
            va="bottom",
            fontsize=9,
        )

    for i, (nc, ca) in enumerate(zip(nc_ticks, ca_ticks)):
        speedup = nc / ca
        ax.text(
            px[i],
            max(nc, ca) / 1e9 * 1.08,
            f"{speedup:.1f}×",
            ha="center",
            va="bottom",
            fontsize=11,
            fontweight="bold",
            color="darkgreen",
        )

    ax.set_xticks(px)
    ax.set_xticklabels([lbl for _, _, lbl in panel_configs])
    ax.set_ylabel("simTicks (×10⁹)")
    ax.set_title(title)
    ax.legend()
    ax.grid(axis="y", alpha=0.3)

plt.tight_layout()
plt.savefig(f"{PLOTS_DIR}/exp6_cache_log.png", dpi=150)
print("Saved exp6_cache_log.png")

# ── Plot 3: accel speedup over CPU at each cache setting ─────────────────────
fig, ax = plt.subplots(figsize=(8, 5))
fig.suptitle("Experiment 6: Accelerator Speedup vs CPU\n(same cache config)")

cache_labels = ["No cache", "L1+L2 cache"]
timing_speedup = [
    get("cpu", "timing", "no")["simTicks"]
    / get("accel", "timing", "no")["simTicks"],
    get("cpu", "timing", "yes")["simTicks"]
    / get("accel", "timing", "yes")["simTicks"],
]
o3_speedup = [
    get("cpu", "o3", "no")["simTicks"] / get("accel", "o3", "no")["simTicks"],
    get("cpu", "o3", "yes")["simTicks"]
    / get("accel", "o3", "yes")["simTicks"],
]

x2 = np.arange(len(cache_labels))
bars1 = ax.bar(
    x2 - w / 2, timing_speedup, w, label="TIMING CPU", color="steelblue"
)
bars2 = ax.bar(x2 + w / 2, o3_speedup, w, label="O3 CPU", color="darkorange")

for bar, v in zip(list(bars1) + list(bars2), timing_speedup + o3_speedup):
    ax.text(
        bar.get_x() + bar.get_width() / 2,
        bar.get_height() + 0.3,
        f"{v:.1f}×",
        ha="center",
        va="bottom",
        fontsize=10,
        fontweight="bold",
    )

ax.set_xticks(x2)
ax.set_xticklabels(cache_labels)
ax.set_ylabel("Speedup (CPU simTicks / Accel simTicks)")
ax.set_title("How much faster is the accelerator than CPU?")
ax.legend()
ax.grid(axis="y", alpha=0.3)

plt.tight_layout()
plt.savefig(f"{PLOTS_DIR}/exp6_accel_speedup.png", dpi=150)
print("Saved exp6_accel_speedup.png")

# ── Plot 4: speedup ladder — all configs vs CPU TIMING no-cache baseline ──────
fig, ax = plt.subplots(figsize=(11, 6))
fig.suptitle(
    "Experiment 6: Speedup Ladder\n" "(baseline = CPU TIMING, no cache)",
    fontsize=12,
)

baseline = get("cpu", "timing", "no")["simTicks"]

ladder = [
    (
        "CPU TIMING\nno cache",
        get("cpu", "timing", "no")["simTicks"],
        "steelblue",
    ),
    ("CPU O3\nno cache", get("cpu", "o3", "no")["simTicks"], "steelblue"),
    (
        "Accel TIMING\nno cache",
        get("accel", "timing", "no")["simTicks"],
        "darkorange",
    ),
    (
        "CPU TIMING\n+ cache",
        get("cpu", "timing", "yes")["simTicks"],
        "steelblue",
    ),
    ("CPU O3\n+ cache", get("cpu", "o3", "yes")["simTicks"], "steelblue"),
    ("Accel O3\nno cache", get("accel", "o3", "no")["simTicks"], "darkorange"),
    (
        "Accel TIMING\n+ cache",
        get("accel", "timing", "yes")["simTicks"],
        "darkorange",
    ),
    ("Accel O3\n+ cache", get("accel", "o3", "yes")["simTicks"], "darkorange"),
]

labels = [l for l, _, _ in ladder]
speedups = [baseline / t for _, t, _ in ladder]
colors = [c for _, _, c in ladder]

bars = ax.bar(
    range(len(ladder)),
    speedups,
    color=colors,
    edgecolor="white",
    linewidth=0.5,
)

for bar, sp in zip(bars, speedups):
    label_text = f"{sp:.0f}×" if sp >= 10 else f"{sp:.1f}×"
    ax.text(
        bar.get_x() + bar.get_width() / 2,
        bar.get_height() * 1.04,
        label_text,
        ha="center",
        va="bottom",
        fontsize=9,
        fontweight="bold",
    )

ax.set_yscale("log")
ax.set_yticks([1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000])
ax.set_yticklabels(
    [
        "1×",
        "2×",
        "5×",
        "10×",
        "20×",
        "50×",
        "100×",
        "200×",
        "500×",
        "1000×",
        "2000×",
    ]
)
ax.set_xticks(range(len(ladder)))
ax.set_xticklabels(labels, fontsize=9)
ax.set_ylabel("Speedup vs CPU TIMING (no cache)")
ax.set_title(
    "Every optimisation step — blue = CPU only, orange = with accelerator"
)
ax.axhline(1, color="gray", linewidth=0.8, linestyle="--")
ax.grid(axis="y", alpha=0.25, which="major")

from matplotlib.patches import Patch

ax.legend(
    handles=[
        Patch(color="steelblue", label="CPU only"),
        Patch(color="darkorange", label="With accelerator"),
    ],
    loc="upper left",
)

plt.tight_layout()
plt.savefig(f"{PLOTS_DIR}/exp6_speedup_ladder.png", dpi=150)
print("Saved exp6_speedup_ladder.png")

# summary
print("\n=== Summary ===")
for m, c, label in configs:
    nc = get(m, c, "no")["simTicks"]
    ca = get(m, c, "yes")["simTicks"]
    print(
        f"{label.replace(chr(10),' '):15s}  no_cache={nc/1e9:8.3f}B  cache={ca/1e9:8.3f}B  speedup={nc/ca:.1f}x"
    )
