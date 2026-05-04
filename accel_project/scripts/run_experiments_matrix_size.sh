#!/bin/bash
# Experiment 7: Matrix size sweep — N=128 and N=256
# Compares CPU vs Accelerator with and without cache for larger matrices

GEM5="./build/RISCV/gem5.fast"
CONFIG="accel_project/configs/run_experiment.py"
BIN_DIR="accel_project/bin"
RESULTS_DIR="accel_project/results"
STATS_DIR="$RESULTS_DIR/stats"
CSV_DIR="$RESULTS_DIR/csv"
CSV_FILE="$CSV_DIR/results_matrix_size.csv"

mkdir -p "$STATS_DIR" "$CSV_DIR" "$RESULTS_DIR/plots"

echo "mode,cpu_type,matrix_n,block_size,data_type,compute_latency,cache,simTicks,simInsts,simSeconds" > "$CSV_FILE"

run_sim() {
    local mode=$1      # accel | cpu
    local cpu=$2       # timing | o3
    local n=$3         # 128 | 256
    local cache=$4     # yes | no

    local binary="${BIN_DIR}/bench_${mode}_b64_float_n${n}"
    local tag="${mode}_${cpu}_b64_float_n${n}"
    [ "$cache" = "yes" ] && tag="${tag}_cache"
    local outdir="${STATS_DIR}/${tag}"

    echo "--- Running: $tag ---"

    local cache_flag=""
    [ "$cache" = "yes" ] && cache_flag="--cache"

    if [ "$mode" = "accel" ]; then
        $GEM5 --outdir="$outdir" $CONFIG \
            --cpu-type "$cpu" \
            --compute-latency 500 \
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

    echo "$mode,$cpu,$n,64,float,500,$cache,$ticks,$insts,$secs" >> "$CSV_FILE"
    echo "    simTicks=$ticks simSeconds=$secs"
}

echo "=== N=128: all 8 configurations ==="
for cpu in timing o3; do
    for cache in no yes; do
        run_sim cpu   $cpu 128 $cache
        run_sim accel $cpu 128 $cache
    done
done

echo ""
echo "=== N=256: all configurations (CPU no-cache may take ~10 min) ==="
for cpu in timing o3; do
    for cache in no yes; do
        run_sim cpu   $cpu 256 $cache
        run_sim accel $cpu 256 $cache
    done
done

echo ""
echo "=== N=512: all configurations (CPU no-cache may take 2-3 hours) ==="
for cpu in timing o3; do
    for cache in no yes; do
        run_sim cpu   $cpu 512 $cache
        run_sim accel $cpu 512 $cache
    done
done

echo ""
echo "=== Done. Results saved to $CSV_FILE ==="
cat "$CSV_FILE"
