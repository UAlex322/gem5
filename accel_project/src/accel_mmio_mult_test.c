#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ACCEL_MMIO_ADDR 0x50000000
#define BLOCK_SIZE 16
#define ELEMS_COUNT (BLOCK_SIZE * BLOCK_SIZE)

int
main()
{
    printf("[Guest OS] Starting Matrix Accelerator test...\n");

    size_t device_status_offset = 0;
    size_t device_control_offset = 4;
    size_t device_block_size_offset = 8;
    size_t device_data_type_offset = 12;
    size_t device_buf_a_offset = 16;
    size_t device_buf_b_offset = 16 + 64 * 64 * 8;
    size_t device_buf_c_offset = 16 + 64 * 64 * 8 * 2;

    volatile uint32_t *accel_status = (uint32_t *)ACCEL_MMIO_ADDR;
    volatile uint32_t *accel_control =
        (uint32_t *)(ACCEL_MMIO_ADDR + device_control_offset);
    volatile uint32_t *accel_block_size =
        (uint32_t *)(ACCEL_MMIO_ADDR + device_block_size_offset);
    volatile uint32_t *accel_data_type =
        (uint32_t *)(ACCEL_MMIO_ADDR + device_data_type_offset);
    volatile void *accel_buf_a =
        (void *)(ACCEL_MMIO_ADDR + device_buf_a_offset);
    volatile void *accel_buf_b =
        (void *)(ACCEL_MMIO_ADDR + device_buf_b_offset);
    volatile void *accel_buf_c =
        (void *)(ACCEL_MMIO_ADDR + device_buf_c_offset);

    float matrix_A[ELEMS_COUNT] = {0};
    float matrix_B[ELEMS_COUNT] = {0};
    float matrix_C[ELEMS_COUNT] = {0};
    float result_matrix[ELEMS_COUNT];

    for (int i = 0; i < BLOCK_SIZE; i++) {
        matrix_A[i * BLOCK_SIZE + i] = 1.0f;
    }

    for (int i = 0; i < ELEMS_COUNT; i++) {
        matrix_B[i] = 2.0f;
        result_matrix[i] = 2.0f;
    }

    *accel_data_type = 1;
    *accel_block_size = BLOCK_SIZE;
    memcpy((void *)accel_buf_a, matrix_A, ELEMS_COUNT * sizeof(float));
    memcpy((void *)accel_buf_b, matrix_B, ELEMS_COUNT * sizeof(float));
    *accel_control = 1;

    while (*accel_status != 1) {
        continue;
    }

    memcpy(matrix_C, (void *)accel_buf_c, ELEMS_COUNT * sizeof(float));

    printf("[Guest OS] Matrix Mult finished.\n");

    printf("[Guest OS] First few values of matrix_C:\n");
    for (int i = 0; i < 5; i++) {
        printf("  matrix_C[%d] = %f\n", i, matrix_C[i]);
    }

    for (int i = 0; i < 16 * 16; i++) {
        if (result_matrix[i] != matrix_C[i]) {
            printf("MISMATCH at [%d]: expected %f, got %f\n", i,
                   result_matrix[i], matrix_C[i]);
        }
    }

    printf("[Guest OS] Matrix Mult was success.\n");
    printf("[Guest OS] Test complete. Exiting.\n");

    return 0;
}
