#include <stdint.h>
#include <stdio.h>

#define ACCEL_MMIO_ADDR 0x50000000

int
main()
{
    printf("[Guest OS] Starting Matrix Accelerator test...\n");

    volatile uint32_t *accel_reg = (uint32_t *)ACCEL_MMIO_ADDR;

    printf("[Guest OS] Writing to accelerator at 0x%X\n", ACCEL_MMIO_ADDR);
    *accel_reg = 0xDEADBEEF;

    volatile uint32_t *status_reg = (uint32_t *)(ACCEL_MMIO_ADDR + 0x04);

    printf("[Guest OS] Reading from accelerator offset 0x04...\n");
    uint32_t result = *status_reg;

    printf("[Guest OS] Read value from accelerator: 0x%X\n", result);

    printf("[Guest OS] Test complete. Exiting.\n");
    return 0;
}
