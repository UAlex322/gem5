#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ── Matrix size ─────────────────────────────────────────── */
#ifndef MATRIX_N
#define MATRIX_N 64
#endif

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 16
#endif

/* ── Data type ───────────────────────────────────────────── */
#ifdef DATA_INT
typedef int elem_t;
#define DATA_TYPE_REG 0
#define FMT "d"
#elif defined(DATA_DOUBLE)
typedef double elem_t;
#define DATA_TYPE_REG 2
#define FMT "f"
#else /* default: float */
typedef float elem_t;
#define DATA_TYPE_REG 1
#define FMT "f"
#endif

/* ── Accelerator MMIO ────────────────────────────────────── */
#define ACCEL_BASE 0x50000000UL
#define REG_STATUS (*(volatile uint32_t *)(ACCEL_BASE + 0x00))
#define REG_CONTROL (*(volatile uint32_t *)(ACCEL_BASE + 0x04))
#define REG_ADDR_A (*(volatile uint64_t *)(ACCEL_BASE + 0x08))
#define REG_ADDR_B (*(volatile uint64_t *)(ACCEL_BASE + 0x10))
#define REG_ADDR_C (*(volatile uint64_t *)(ACCEL_BASE + 0x18))
#define REG_BLOCK_SIZE (*(volatile uint32_t *)(ACCEL_BASE + 0x20))
#define REG_DATA_TYPE (*(volatile uint32_t *)(ACCEL_BASE + 0x24))

#define STATUS_DONE 1

/* ── Matrices (static to avoid large stack allocation) ───── */
static elem_t A[MATRIX_N][MATRIX_N];
static elem_t B[MATRIX_N][MATRIX_N];
static elem_t C[MATRIX_N][MATRIX_N];

/* ── Helper: init matrices ───────────────────────────────── */
static void
init_matrices(void)
{
    for (int i = 0; i < MATRIX_N; i++) {
        for (int j = 0; j < MATRIX_N; j++) {
            A[i][j] = (elem_t)(i + j + 1);
            B[i][j] = (elem_t)(i == j ? 1 : 0); /* identity */
        }
    }
    memset(C, 0, sizeof(C));
}

/* ── Helper: verify C == A (B is identity) ───────────────── */
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

/* ═══════════════════════════════════════════════════════════
 * CPU-only path
 * ═══════════════════════════════════════════════════════════ */
#ifndef USE_ACCEL

static void
matmul_cpu(void)
{
    for (int i = 0; i < MATRIX_N; i++) {
        for (int j = 0; j < MATRIX_N; j++) {
            for (int k = 0; k < MATRIX_N; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

int
main(void)
{
    printf("bench_matmul: CPU-only N=%d type=%d\n", MATRIX_N, DATA_TYPE_REG);
    init_matrices();
    matmul_cpu();
    printf("verify: %s\n", verify() ? "OK" : "FAIL");
    return 0;
}

/* ═══════════════════════════════════════════════════════════
 * Accelerator path
 * ═══════════════════════════════════════════════════════════ */
#else /* USE_ACCEL */

/* Contiguous buffers for accelerator input/output */
static elem_t buf_a[BLOCK_SIZE][BLOCK_SIZE];
static elem_t buf_b[BLOCK_SIZE][BLOCK_SIZE];
static elem_t buf_c[BLOCK_SIZE][BLOCK_SIZE];

/* Run one BLOCK_SIZE×BLOCK_SIZE multiplication: buf_c = buf_a * buf_b */
static void
accel_run(void)
{
    REG_DATA_TYPE = DATA_TYPE_REG;
    REG_BLOCK_SIZE = BLOCK_SIZE;
    REG_ADDR_A = (uint64_t)buf_a;
    REG_ADDR_B = (uint64_t)buf_b;
    REG_ADDR_C = (uint64_t)buf_c;
    REG_CONTROL = 1;
    while (REG_STATUS != STATUS_DONE)
        ;
}

/*
 * Blocked matmul: C = A * B
 * For each output block C[i][j], accumulate over k:
 *   buf_a = copy of A[i][k] block  (CPU, contiguous)
 *   buf_b = copy of B[k][j] block  (CPU, contiguous)
 *   buf_c = buf_a * buf_b           (accelerator)
 *   C[i][j] += buf_c               (CPU accumulation)
 */
static void
matmul_accel(void)
{
    const int P = MATRIX_N / BLOCK_SIZE;

    for (int i = 0; i < P; i++) {
        for (int j = 0; j < P; j++) {
            for (int k = 0; k < P; k++) {
                /* copy A[i][k] block into contiguous buf_a */
                for (int r = 0; r < BLOCK_SIZE; r++) {
                    for (int c = 0; c < BLOCK_SIZE; c++) {
                        buf_a[r][c] =
                            A[i * BLOCK_SIZE + r][k * BLOCK_SIZE + c];
                    }
                }

                /* copy B[k][j] block into contiguous buf_b */
                for (int r = 0; r < BLOCK_SIZE; r++) {
                    for (int c = 0; c < BLOCK_SIZE; c++) {
                        buf_b[r][c] =
                            B[k * BLOCK_SIZE + r][j * BLOCK_SIZE + c];
                    }
                }

                memset(buf_c, 0, sizeof(buf_c));
                accel_run();

                /* accumulate buf_c into C[i][j] block */
                for (int r = 0; r < BLOCK_SIZE; r++) {
                    for (int c = 0; c < BLOCK_SIZE; c++) {
                        C[i * BLOCK_SIZE + r][j * BLOCK_SIZE + c] +=
                            buf_c[r][c];
                    }
                }
            }
        }
    }
}

int
main(void)
{
    printf("bench_matmul: ACCEL N=%d block=%d type=%d\n", MATRIX_N, BLOCK_SIZE,
           DATA_TYPE_REG);
    init_matrices();
    matmul_accel();
    printf("verify: %s\n", verify() ? "OK" : "FAIL");
    return 0;
}

#endif /* USE_ACCEL */
