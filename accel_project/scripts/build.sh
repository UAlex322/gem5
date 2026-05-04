#!/bin/bash

PROJECT_DIR="accel_project"
SRC_DIR="$PROJECT_DIR/src"
BIN_DIR="$PROJECT_DIR/bin"
COMPILER="riscv64-unknown-elf-gcc"
FLAGS="-static -O2"

mkdir -p $BIN_DIR

echo "=== Building bench_matmul ==="

for BLOCK_SIZE in 16 32 64; do
    for DATA_TYPE in FLOAT INT DOUBLE; do
        case $DATA_TYPE in
            INT)    DT_FLAG="-DDATA_INT"    ;;
            FLOAT)  DT_FLAG=""              ;;
            DOUBLE) DT_FLAG="-DDATA_DOUBLE" ;;
        esac

        # CPU-only
        NAME="bench_cpu_b${BLOCK_SIZE}_${DATA_TYPE,,}"
        echo "Building $NAME..."
        $COMPILER $FLAGS \
            -DMATRIX_N=64 -DBLOCK_SIZE=$BLOCK_SIZE $DT_FLAG \
            -o $BIN_DIR/$NAME \
            $SRC_DIR/bench_matmul.c

        # Accelerator
        NAME="bench_accel_b${BLOCK_SIZE}_${DATA_TYPE,,}"
        echo "Building $NAME..."
        $COMPILER $FLAGS \
            -DUSE_ACCEL -DMATRIX_N=64 -DBLOCK_SIZE=$BLOCK_SIZE $DT_FLAG \
            -o $BIN_DIR/$NAME \
            $SRC_DIR/bench_matmul.c
    done
done

echo ""
echo "=== Building bench_matmul N=128 and N=256 (block=64, float) ==="

for N in 128 256 512; do
    NAME="bench_cpu_b64_float_n${N}"
    echo "Building $NAME..."
    $COMPILER $FLAGS \
        -DMATRIX_N=$N -DBLOCK_SIZE=64 \
        -o $BIN_DIR/$NAME \
        $SRC_DIR/bench_matmul.c

    NAME="bench_accel_b64_float_n${N}"
    echo "Building $NAME..."
    $COMPILER $FLAGS \
        -DUSE_ACCEL -DMATRIX_N=$N -DBLOCK_SIZE=64 \
        -o $BIN_DIR/$NAME \
        $SRC_DIR/bench_matmul.c
done

echo ""
echo "=== Building bench_multi_accel ==="

for NUM_ACCELS in 1 2 4; do
    NAME="bench_multi_n${NUM_ACCELS}_b64_float"
    echo "Building $NAME..."
    $COMPILER $FLAGS \
        -DMATRIX_N=64 -DBLOCK_SIZE=16 -DNUM_ACCELS=$NUM_ACCELS \
        -o $BIN_DIR/$NAME \
        $SRC_DIR/bench_multi_accel.c
done

echo ""
echo "=== Building legacy tests ==="
for PROGRAM in simple_mult_test accel_test accel_mmio_mult_test; do
    if [ -f "$SRC_DIR/${PROGRAM}.c" ]; then
        echo "Building ${PROGRAM}..."
        $COMPILER -static -O2 -o $BIN_DIR/${PROGRAM}_O2 $SRC_DIR/${PROGRAM}.c
    fi
done

echo ""
echo "=== Done. Binaries in $BIN_DIR: ==="
ls -lh $BIN_DIR/
