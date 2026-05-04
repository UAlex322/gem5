#!/bin/bash

GEM5="./build/RISCV/gem5.opt"
CONFIG="accel_project/configs/run_experiment.py"
BIN_DIR="accel_project/bin"
RESULTS_DIR="accel_project/results"
STATS_DIR="$RESULTS_DIR/stats"
CSV_DIR="$RESULTS_DIR/csv"
CSV_FILE="$CSV_DIR/results_multi.csv"

mkdir -p "$STATS_DIR" "$CSV_DIR" "accel_project/results/plots"

echo "num_accels,simTicks,simInsts,simSeconds" > "$CSV_FILE"

run_sim() {
    local num_accels=$1

    local binary="${BIN_DIR}/bench_multi_n${num_accels}_b64_float"
    local tag="multi_n${num_accels}_b64_float"
    local outdir="${STATS_DIR}/${tag}"

    echo "--- Running: $tag ---"

    $GEM5 --outdir="$outdir" $CONFIG \
        --cpu-type timing \
        --compute-latency 500 \
        --num-accels "$num_accels" \
        --binary "$binary" \
        > /dev/null 2>&1

    local stats="$outdir/stats.txt"
    local ticks=$(grep "^simTicks" "$stats" | awk '{print $2}')
    local insts=$(grep "^simInsts" "$stats" | awk '{print $2}')
    local secs=$(grep "^simSeconds" "$stats" | awk '{print $2}')

    echo "$num_accels,$ticks,$insts,$secs" >> "$CSV_FILE"
    echo "    simTicks=$ticks simInsts=$insts simSeconds=$secs"
}

echo "=== Multi-accelerator sweep (TIMING, block=16, float, latency=500) ==="
for n in 1 2 4; do
    run_sim $n
done

echo ""
echo "=== Done. Results saved to $CSV_FILE ==="
cat "$CSV_FILE"
