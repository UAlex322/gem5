#!/bin/bash
# Experiment 6: CPU vs Accelerator with L1+L2 cache
# Runs 8 configurations: 2 modes x 2 cpu_types x 2 cache settings

GEM5="./build/RISCV/gem5.fast"
CONFIG="accel_project/configs/run_experiment.py"
BIN_DIR="accel_project/bin"
RESULTS_DIR="accel_project/results"
STATS_DIR="$RESULTS_DIR/stats"
CSV_DIR="$RESULTS_DIR/csv"
CSV_FILE="$CSV_DIR/results_cache.csv"

mkdir -p "$STATS_DIR" "$CSV_DIR" "$RESULTS_DIR/plots"

echo "mode,cpu_type,block_size,data_type,compute_latency,cache,simTicks,simInsts,simSeconds" > "$CSV_FILE"

run_sim() {
    local mode=$1      # accel | cpu
    local cpu=$2       # timing | o3
    local block=$3     # 64
    local dtype=$4     # float
    local latency=$5   # 500 | -
    local cache=$6     # yes | no

    local binary="${BIN_DIR}/bench_${mode}_b${block}_${dtype}"
    local tag="${mode}_${cpu}_b${block}_${dtype}"
    [ "$mode" = "accel" ] && tag="${tag}_lat${latency}"
    [ "$cache" = "yes" ] && tag="${tag}_cache"
    local outdir="${STATS_DIR}/${tag}"

    echo "--- Running: $tag ---"

    local cache_flag=""
    [ "$cache" = "yes" ] && cache_flag="--cache"

    if [ "$mode" = "accel" ]; then
        $GEM5 --outdir="$outdir" $CONFIG \
            --cpu-type "$cpu" \
            --compute-latency "$latency" \
            --num-accels 1 \
            $cache_flag \
            --binary "$binary" \
            > /dev/null 2>&1
    else
        $GEM5 --outdir="$outdir" $CONFIG \
            --cpu-type "$cpu" \
            --num-accels 0 \
            $cache_flag \
            --binary "$binary" \
            > /dev/null 2>&1
    fi

    local stats="$outdir/stats.txt"
    local ticks=$(grep "^simTicks" "$stats" | awk '{print $2}')
    local insts=$(grep "^simInsts" "$stats" | awk '{print $2}')
    local secs=$(grep "^simSeconds" "$stats" | awk '{print $2}')

    echo "$mode,$cpu,$block,$dtype,$latency,$cache,$ticks,$insts,$secs" >> "$CSV_FILE"
    echo "    simTicks=$ticks simSeconds=$secs"
}

echo "=== Experiment 6: CPU vs Accelerator, with and without cache (block=64, float) ==="

for cpu in timing o3; do
    for cache in no yes; do
        run_sim cpu   $cpu 64 float -   $cache
        run_sim accel $cpu 64 float 500 $cache
    done
done

echo ""
echo "=== Done. Results saved to $CSV_FILE ==="
cat "$CSV_FILE"
