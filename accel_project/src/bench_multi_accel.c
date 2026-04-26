#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef MATRIX_N
#define MATRIX_N 64
#endif

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 16
#endif

#ifndef NUM_ACCELS
#define NUM_ACCELS 1
#endif

#ifdef DATA_INT
typedef int elem_t;
#define DATA_TYPE_REG 0
#define FMT "d"
#elif defined(DATA_DOUBLE)
typedef double elem_t;
#define DATA_TYPE_REG 2
#define FMT "f"
#else
typedef float elem_t;
#define DATA_TYPE_REG 1
#define FMT "f"
#endif

#define ACCEL_STEP 0x01000000UL
#define ACCEL_BASE(n) (0x50000000UL + (n) * ACCEL_STEP)

#define REG_STATUS(n) (*(volatile uint32_t *)(ACCEL_BASE(n) + 0x00))
#define REG_CONTROL(n) (*(volatile uint32_t *)(ACCEL_BASE(n) + 0x04))
#define REG_ADDR_A(n) (*(volatile uint64_t *)(ACCEL_BASE(n) + 0x08))
#define REG_ADDR_B(n) (*(volatile uint64_t *)(ACCEL_BASE(n) + 0x10))
#define REG_ADDR_C(n) (*(volatile uint64_t *)(ACCEL_BASE(n) + 0x18))
#define REG_BLOCK_SIZE(n) (*(volatile uint32_t *)(ACCEL_BASE(n) + 0x20))
#define REG_DATA_TYPE(n) (*(volatile uint32_t *)(ACCEL_BASE(n) + 0x24))

#define STATUS_DONE 1

static elem_t A[MATRIX_N][MATRIX_N];
static elem_t B[MATRIX_N][MATRIX_N];
static elem_t C[MATRIX_N][MATRIX_N];

static elem_t buf_a[NUM_ACCELS][BLOCK_SIZE][BLOCK_SIZE];
static elem_t buf_b[NUM_ACCELS][BLOCK_SIZE][BLOCK_SIZE];
static elem_t buf_c[NUM_ACCELS][BLOCK_SIZE][BLOCK_SIZE];

static void
init_matrices(void)
{
    for (int i = 0; i < MATRIX_N; i++) {
        for (int j = 0; j < MATRIX_N; j++) {
            A[i][j] = (elem_t)(i + j + 1);
            B[i][j] = (elem_t)(i == j ? 1 : 0);
        }
    }
    memset(C, 0, sizeof(C));
}

static int
verify(void)
{
    for (int i = 0; i < MATRIX_N; i++) {
        for (int j = 0; j < MATRIX_N; j++) {
            if (C[i][j] != A[i][j]) {
                printf("MISMATCH at [%d][%d]: expected %" FMT " got %" FMT
                       "\n",
                       i, j, A[i][j], C[i][j]);
                return 0;
            }
        }
    }
    return 1;
}

static void
launch(int n, int i, int ki, int j)
{
    for (int r = 0; r < BLOCK_SIZE; r++) {
        for (int c = 0; c < BLOCK_SIZE; c++) {
            buf_a[n][r][c] = A[i * BLOCK_SIZE + r][ki * BLOCK_SIZE + c];
            buf_b[n][r][c] = B[ki * BLOCK_SIZE + r][j * BLOCK_SIZE + c];
        }
    }
    memset(buf_c[n], 0, sizeof(buf_c[n]));
    REG_DATA_TYPE(n) = DATA_TYPE_REG;
    REG_BLOCK_SIZE(n) = BLOCK_SIZE;
    REG_ADDR_A(n) = (uint64_t)buf_a[n];
    REG_ADDR_B(n) = (uint64_t)buf_b[n];
    REG_ADDR_C(n) = (uint64_t)buf_c[n];
    REG_CONTROL(n) = 1;
}

static void
accumulate(int n, int i, int j)
{
    for (int r = 0; r < BLOCK_SIZE; r++) {
        for (int c = 0; c < BLOCK_SIZE; c++) {
            C[i * BLOCK_SIZE + r][j * BLOCK_SIZE + c] += buf_c[n][r][c];
        }
    }
}

static void
matmul_multi_accel(void)
{
    const int P = MATRIX_N / BLOCK_SIZE;
    int ki_assigned[NUM_ACCELS];

    for (int i = 0; i < P; i++) {
        for (int j = 0; j < P; j++) {
            int next_k = 0;
            int done_count = 0;

            for (int n = 0; n < NUM_ACCELS; n++) {
                ki_assigned[n] = -1;
            }

            while (done_count < P) {
                for (int n = 0; n < NUM_ACCELS; n++) {
                    /* if running and done — accumulate, free slot */
                    if (ki_assigned[n] >= 0 && REG_STATUS(n) == STATUS_DONE) {
                        accumulate(n, i, j);
                        ki_assigned[n] = -1;
                        done_count++;
                    }
                    /* if free and work remains — dispatch next block */
                    if (ki_assigned[n] < 0 && next_k < P) {
                        ki_assigned[n] = next_k;
                        launch(n, i, next_k, j);
                        next_k++;
                    }
                }
            }
        }
    }
}

int
main(void)
{
    printf("bench_multi_accel: N=%d block=%d num_accels=%d type=%d\n",
           MATRIX_N, BLOCK_SIZE, NUM_ACCELS, DATA_TYPE_REG);
    init_matrices();
    matmul_multi_accel();
    printf("verify: %s\n", verify() ? "OK" : "FAIL");
    return 0;
}
