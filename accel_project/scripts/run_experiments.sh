#!/bin/bash

GEM5="./build/RISCV/gem5.opt"
CONFIG="accel_project/configs/run_experiment.py"
BIN_DIR="accel_project/bin"
RESULTS_DIR="accel_project/results"
STATS_DIR="$RESULTS_DIR/stats"
CSV_DIR="$RESULTS_DIR/csv"
CSV_FILE="$CSV_DIR/results.csv"

mkdir -p "$STATS_DIR" "$CSV_DIR" "accel_project/results/plots"

# CSV header
echo "mode,cpu_type,block_size,data_type,compute_latency,simTicks,simInsts,simSeconds" > "$CSV_FILE"

run_sim() {
    local mode=$1        # accel | cpu
    local cpu=$2         # timing | o3
    local block=$3       # 16 | 32 | 64
    local dtype=$4       # float | int | double
    local latency=$5     # 500 | 100 | 2000 | -

    local binary="${BIN_DIR}/bench_${mode}_b${block}_${dtype}"
    local tag="${mode}_${cpu}_b${block}_${dtype}"
    [ "$mode" = "accel" ] && tag="${tag}_lat${latency}"
    local outdir="${STATS_DIR}/${tag}"

    echo "--- Running: $tag ---"

    if [ "$mode" = "accel" ]; then
        $GEM5 --outdir="$outdir" $CONFIG \
            --cpu-type "$cpu" \
            --compute-latency "$latency" \
            --num-accels 1 \
            --binary "$binary" \
            > /dev/null 2>&1
    else
        $GEM5 --outdir="$outdir" $CONFIG \
            --cpu-type "$cpu" \
            --num-accels 0 \
            --binary "$binary" \
            > /dev/null 2>&1
    fi

    # extract metrics from stats.txt
    local stats="$outdir/stats.txt"
    local ticks=$(grep "^simTicks" "$stats" | awk '{print $2}')
    local insts=$(grep "^simInsts" "$stats" | awk '{print $2}')
    local secs=$(grep "^simSeconds" "$stats" | awk '{print $2}')

    echo "$mode,$cpu,$block,$dtype,$latency,$ticks,$insts,$secs" >> "$CSV_FILE"
    echo "    simTicks=$ticks simInsts=$insts simSeconds=$secs"
}

# ── Experiment 1: accel vs CPU, vary block_size, TIMING ──────
echo "=== Experiment 1: block_size sweep (TIMING, float, latency=500) ==="
for block in 16 32 64; do
    run_sim accel timing $block float 500
    run_sim cpu   timing $block float -
done

# ── Experiment 2: CPU type comparison, block=64 ───────────────
echo "=== Experiment 2: CPU type (block=64, float, latency=500) ==="
run_sim accel o3 64 float 500
run_sim cpu   o3 64 float -

# ── Experiment 3: compute_latency sweep, TIMING, block=64 ─────
echo "=== Experiment 3: latency sweep (TIMING, block=64, float) ==="
for latency in 100 500 2000 10000; do
    run_sim accel timing 64 float $latency
done

# ── Experiment 4: data type comparison, TIMING, block=32 ──────
echo "=== Experiment 4: data type (TIMING, block=32, latency=500) ==="
for dtype in float int double; do
    run_sim accel timing 32 $dtype 500
    run_sim cpu   timing 32 $dtype -
done

echo ""
echo "=== Done. Results saved to $CSV_FILE ==="
cat "$CSV_FILE"
