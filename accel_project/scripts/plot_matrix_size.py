import csv
import os

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Patch

CACHE_CSV = "accel_project/results/csv/results_cache.csv"
MATRIX_CSV = "accel_project/results/csv/results_matrix_size.csv"
PLOTS_DIR = "accel_project/results/plots"

os.makedirs(PLOTS_DIR, exist_ok=True)

# ── Load data ─────────────────────────────────────────────────────────────────
rows = []

with open(CACHE_CSV) as f:
    for r in csv.DictReader(f):
        r["matrix_n"] = 64
        r["simTicks"] = int(r["simTicks"])
        r["simInsts"] = int(r["simInsts"])
        rows.append(r)

with open(MATRIX_CSV) as f:
    for r in csv.DictReader(f):
        r["matrix_n"] = int(r["matrix_n"])
        r["simTicks"] = int(r["simTicks"])
        r["simInsts"] = int(r["simInsts"])
        rows.append(r)


def get(mode, cpu, n, cache):
    for r in rows:
        if (
            r["mode"] == mode
            and r["cpu_type"] == cpu
            and r["matrix_n"] == n
            and r["cache"] == cache
        ):
            return r
    return None


configs = [
    ("cpu", "timing", "CPU TIMING"),
    ("cpu", "o3", "CPU O3"),
    ("accel", "timing", "Accel TIMING"),
    ("accel", "o3", "Accel O3"),
]

# ── Plot 1: Cache speedup vs matrix size (line plot) ─────────────────────────
# How much does cache help as matrix grows?
fig, ax = plt.subplots(figsize=(9, 6))
fig.suptitle(
    "Experiment 7: Cache Speedup vs Matrix Size\n"
    "(speedup = no_cache ticks / cache ticks)",
    fontsize=12,
)

ns = [64, 128, 256, 512]
markers = ["o", "s", "^", "D"]
line_colors = ["steelblue", "royalblue", "darkorange", "orangered"]

for (mode, cpu, label), marker, color in zip(configs, markers, line_colors):
    speedups = []
    for n in ns:
        nc = get(mode, cpu, n, "no")
        ca = get(mode, cpu, n, "yes")
        if nc and ca:
            speedups.append(nc["simTicks"] / ca["simTicks"])
        else:
            speedups.append(None)
    valid = [(n, s) for n, s in zip(ns, speedups) if s is not None]
    if valid:
        xs, ys = zip(*valid)
        ax.plot(
            xs,
            ys,
            marker=marker,
            color=color,
            linewidth=2,
            markersize=8,
            label=label,
        )
        for x, y in zip(xs, ys):
            ax.annotate(
                f"{y:.1f}×",
                (x, y),
                textcoords="offset points",
                xytext=(6, 4),
                fontsize=9,
            )

ax.set_xlabel("Matrix size N")
ax.set_ylabel("Cache speedup (×)")
ax.set_xticks(ns)
ax.set_xticklabels([f"N={n}\n({n}×{n})" for n in ns])
ax.legend()
ax.grid(alpha=0.3)
ax.set_title(
    "CPU cache advantage drops with larger matrices;\nAccelerator stays efficient"
)

plt.tight_layout()
plt.savefig(f"{PLOTS_DIR}/exp7_cache_speedup_vs_n.png", dpi=150)
print("Saved exp7_cache_speedup_vs_n.png")

# ── Plot 2: Accel speedup over CPU vs matrix size ─────────────────────────────
fig, axes = plt.subplots(1, 2, figsize=(12, 6))
fig.suptitle(
    "Experiment 7: Accelerator Speedup vs CPU — by Matrix Size\n"
    "(left = no cache, right = with cache)",
    fontsize=12,
)

for ax, cache, title in zip(axes, ["no", "yes"], ["No cache", "L1+L2 cache"]):
    available_ns = [
        n
        for n in [64, 128, 256, 512]
        if get("accel", "timing", n, cache) and get("cpu", "timing", n, cache)
    ]

    x = np.arange(len(available_ns))
    w = 0.35

    timing_sp = [
        get("cpu", "timing", n, cache)["simTicks"]
        / get("accel", "timing", n, cache)["simTicks"]
        for n in available_ns
    ]
    o3_sp = [
        get("cpu", "o3", n, cache)["simTicks"]
        / get("accel", "o3", n, cache)["simTicks"]
        for n in available_ns
        if get("cpu", "o3", n, cache) and get("accel", "o3", n, cache)
    ]

    b1 = ax.bar(x - w / 2, timing_sp, w, label="TIMING", color="steelblue")
    if len(o3_sp) == len(available_ns):
        b2 = ax.bar(x + w / 2, o3_sp, w, label="O3", color="darkorange")
        for bar, v in zip(b2, o3_sp):
            ax.text(
                bar.get_x() + bar.get_width() / 2,
                bar.get_height() + 0.3,
                f"{v:.1f}×",
                ha="center",
                va="bottom",
                fontsize=9,
                fontweight="bold",
            )

    for bar, v in zip(b1, timing_sp):
        ax.text(
            bar.get_x() + bar.get_width() / 2,
            bar.get_height() + 0.3,
            f"{v:.1f}×",
            ha="center",
            va="bottom",
            fontsize=9,
            fontweight="bold",
        )

    ax.set_xticks(x)
    ax.set_xticklabels([f"N={n}" for n in available_ns])
    ax.set_ylabel("Speedup (CPU ticks / Accel ticks)")
    ax.set_title(title)
    ax.legend()
    ax.grid(axis="y", alpha=0.3)

plt.tight_layout()
plt.savefig(f"{PLOTS_DIR}/exp7_accel_speedup_vs_n.png", dpi=150)
print("Saved exp7_accel_speedup_vs_n.png")

# ── Plot 3: Speedup ladder for N=128 ─────────────────────────────────────────
baseline_128 = get("cpu", "timing", 128, "no")
if baseline_128:
    baseline = baseline_128["simTicks"]
    fig, ax = plt.subplots(figsize=(11, 6))
    fig.suptitle(
        "Experiment 7: Speedup Ladder — N=128\n"
        "(baseline = CPU TIMING, no cache, N=128)",
        fontsize=12,
    )

    ladder = []
    candidates = [
        ("cpu", "timing", "no", "CPU TIMING\nno cache", "steelblue"),
        ("cpu", "o3", "no", "CPU O3\nno cache", "steelblue"),
        ("accel", "timing", "no", "Accel TIMING\nno cache", "darkorange"),
        ("cpu", "timing", "yes", "CPU TIMING\n+ cache", "steelblue"),
        ("cpu", "o3", "yes", "CPU O3\n+ cache", "steelblue"),
        ("accel", "o3", "no", "Accel O3\nno cache", "darkorange"),
        ("accel", "timing", "yes", "Accel TIMING\n+ cache", "darkorange"),
        ("accel", "o3", "yes", "Accel O3\n+ cache", "darkorange"),
    ]
    for mode, cpu, cache, label, color in candidates:
        r = get(mode, cpu, 128, cache)
        if r:
            ladder.append((label, r["simTicks"], color))

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
    ax.set_yticks([1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000])
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
            "5000×",
        ]
    )
    ax.set_xticks(range(len(ladder)))
    ax.set_xticklabels(labels, fontsize=9)
    ax.set_ylabel("Speedup vs CPU TIMING no cache (N=128)")
    ax.axhline(1, color="gray", linewidth=0.8, linestyle="--")
    ax.grid(axis="y", alpha=0.25, which="major")
    ax.legend(
        handles=[
            Patch(color="steelblue", label="CPU only"),
            Patch(color="darkorange", label="With accelerator"),
        ],
        loc="upper left",
    )

    plt.tight_layout()
    plt.savefig(f"{PLOTS_DIR}/exp7_speedup_ladder_n128.png", dpi=150)
    print("Saved exp7_speedup_ladder_n128.png")

# ── Plot 4: Accel speedup over CPU — two panels (cache / no cache) ────────────
fig, (ax_cache, ax_nocache) = plt.subplots(1, 2, figsize=(13, 6))
fig.suptitle(
    "Experiment 7: Accelerator Speedup over CPU by Matrix Size",
    fontsize=12,
)

cpu_types = [
    ("timing", "TIMING", "steelblue", "o"),
    ("o3", "O3", "darkorange", "s"),
]

for ax, cache, title in [
    (ax_cache, "yes", "With L1+L2 cache"),
    (ax_nocache, "no", "No cache"),
]:
    for cpu, label, color, marker in cpu_types:
        speedups = []
        valid_ns = []
        for n in ns:
            cpu_r = get("cpu", cpu, n, cache)
            accel_r = get("accel", cpu, n, cache)
            if cpu_r and accel_r:
                speedups.append(cpu_r["simTicks"] / accel_r["simTicks"])
                valid_ns.append(n)
        if not speedups:
            continue
        ax.plot(
            valid_ns,
            speedups,
            marker=marker,
            color=color,
            linewidth=2,
            markersize=8,
            label=f"vs CPU {label}",
        )
        for n, sp in zip(valid_ns, speedups):
            ax.annotate(
                f"{sp:.1f}×",
                (n, sp),
                textcoords="offset points",
                xytext=(6, 4),
                fontsize=9,
            )

    ax.set_xlabel("Matrix size N")
    ax.set_ylabel("Speedup (CPU ticks / Accel ticks)")
    ax.set_xticks(ns)
    ax.set_xticklabels([f"N={n}\n({n}×{n})" for n in ns])
    ax.legend()
    ax.grid(alpha=0.3)
    ax.set_title(title)

plt.tight_layout()
plt.savefig(f"{PLOTS_DIR}/exp7_accel_vs_cpu_cache.png", dpi=150)
print("Saved exp7_accel_vs_cpu_cache.png")

# ── Summary ───────────────────────────────────────────────────────────────────
print("\n=== Cache speedup by N ===")
for mode, cpu, label in configs:
    parts = []
    for n in [64, 128, 256]:
        nc = get(mode, cpu, n, "no")
        ca = get(mode, cpu, n, "yes")
        if nc and ca:
            parts.append(f"N={n}: {nc['simTicks']/ca['simTicks']:.1f}×")
    print(f"{label:15s}  {',  '.join(parts)}")
