#ifndef __DEV_MATRIX_ACCEL_HH__
#define __DEV_MATRIX_ACCEL_HH__

#include "dev/dma_virt_device.hh"
#include "params/MatrixAccel.hh"
#include "sim/full_system.hh"
#include "sim/system.hh"

namespace gem5
{

class MatrixAccel : public DmaVirtDevice
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

    MatrixAccel(const MatrixAccelParams& p);

    void init() override;
    Tick read(PacketPtr pkt) override;
    Tick write(PacketPtr pkt) override;

    TranslationGenPtr translate(Addr vaddr, Addr size) override;
    AddrRangeList getAddrRanges() const override;

  private:
    System *system;
    Addr pioAddr;
    Addr pioSize;
    Tick pioDelay;
    Cycles computeLatency;

    static constexpr uint32_t STATUS_OFFSET = 0;
    static constexpr uint32_t CONTROL_OFFSET = 4;
    static constexpr uint32_t ADDR_A_OFFSET = 8;
    static constexpr uint32_t ADDR_B_OFFSET = 16;
    static constexpr uint32_t ADDR_C_OFFSET = 24;
    static constexpr uint32_t BLOCK_SIZE_OFFSET = 32;
    static constexpr uint32_t DATA_TYPE_OFFSET = 36;

    Status status{Status::IDLE};
    uint32_t control{};
    uint64_t addr_a{};
    uint64_t addr_b{};
    uint64_t addr_c{};
    uint32_t block_size{16};
    DataType data_type{DataType::INT};

    std::vector<uint8_t> buf_a;
    std::vector<uint8_t> buf_b;
    std::vector<uint8_t> buf_c;

    void fetch_A();
    void fetch_B();
    void compute();
    void write_C();
    void on_done();

    EventFunctionWrapper fetch_A_event;
    EventFunctionWrapper write_C_event;

    template <typename T>
    void
    matrix_mult()
    {
        T *a = reinterpret_cast<T *>(buf_a.data());
        T *b = reinterpret_cast<T *>(buf_b.data());
        T *c = reinterpret_cast<T *>(buf_c.data());
        size_t n = block_size;

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
