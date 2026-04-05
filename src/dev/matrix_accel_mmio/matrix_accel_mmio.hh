#ifndef __DEV_MATRIX_ACCEL_MMIO_HH__
#define __DEV_MATRIX_ACCEL_MMIO_HH__

#include "dev/io_device.hh"
#include "params/MatrixAccelMMIO.hh"

namespace gem5
{

class MatrixAccelMMIO : public BasicPioDevice
{
 public:
    enum class Status : uint32_t
    {
        IDLE = 0,
        DONE = 1,
        BUSY = 2
    };

    enum class DataType : uint32_t
    {
        INT    = 0,
        FLOAT  = 1,
        DOUBLE = 2
    };

    MatrixAccelMMIO(const MatrixAccelMMIOParams& p);

    Tick read(PacketPtr pkt) override;
    Tick write(PacketPtr pkt) override;

 private:
    static constexpr uint32_t STATUS_OFFSET = 0;
    static constexpr uint32_t CONTROL_OFFSET = 4;
    static constexpr uint32_t BLOCK_SIZE_OFFSET = 8;
    static constexpr uint32_t DATA_TYPE_OFFSET = 12;

    static constexpr uint32_t MAX_BLOCK_SIZE = 64;
    static constexpr uint32_t MAX_ELEM_SIZE = sizeof(double);
    static constexpr uint32_t MAX_BUF_SIZE = MAX_BLOCK_SIZE * MAX_BLOCK_SIZE * MAX_ELEM_SIZE;
    
    static constexpr uint32_t BUF_A_OFFSET = 16;
    static constexpr uint32_t BUF_B_OFFSET = BUF_A_OFFSET + MAX_BUF_SIZE;
    static constexpr uint32_t BUF_C_OFFSET = BUF_B_OFFSET + MAX_BUF_SIZE;

    Status status{Status::IDLE};
    uint32_t control{};
    uint32_t block_size{16};
    DataType data_type{DataType::INT};

    uint8_t buf_a[MAX_BUF_SIZE]{};
    uint8_t buf_b[MAX_BUF_SIZE]{};
    uint8_t buf_c[MAX_BUF_SIZE]{};

    void compute();

    EventFunctionWrapper compute_event;

    template<typename T>
    void matrix_mult(uint8_t* raw_a, uint8_t* raw_b,
         uint8_t* raw_c, size_t n) {
        T* a = reinterpret_cast<T*>(raw_a);
        T* b = reinterpret_cast<T*>(raw_b);
        T* c = reinterpret_cast<T*>(raw_c);

        for (size_t i = 0; i < n; i++) {
            for (size_t j = 0; j < n; j++) {
                for (size_t k = 0; k < n; k++) {
                    c[i * n + j] += a[i * n + k] * b[k * n + j];
                }
            }
        }
    }
};

} // namespace gem5

#endif // __DEV_MATRIX_ACCEL_HH__
