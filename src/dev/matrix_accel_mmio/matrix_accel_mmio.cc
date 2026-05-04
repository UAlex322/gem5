#include "dev/matrix_accel_mmio/matrix_accel_mmio.hh"

#include "debug/MatrixAccelMMIO.hh"
#include "mem/packet_access.hh"

namespace gem5
{

MatrixAccelMMIO::MatrixAccelMMIO(const MatrixAccelMMIOParams &p)
    : BasicPioDevice(p, p.pio_size),
      compute_event([this] { compute(); }, name()),
      on_done_event([this] { on_done(); }, name())
{}

Tick
MatrixAccelMMIO::read(PacketPtr pkt)
{
    Addr offset = pkt->getAddr() - pioAddr;

    DPRINTF(MatrixAccelMMIO, "read: offset %d\n", offset);
    panic_if(offset >= pioSize, "out of bounds read in %s", name());

    if (offset == STATUS_OFFSET) {
        pkt->setLE<uint32_t>(static_cast<uint32_t>(status));
    } else if (offset >= BUF_C_OFFSET &&
               offset < BUF_C_OFFSET + MAX_BUF_SIZE) {
        Addr buf_offset = offset - BUF_C_OFFSET;
        memcpy(pkt->getPtr<void>(), buf_c + buf_offset, pkt->getSize());
    } else {
        warn("MatrixAccelMMIO::read: unknown offset %d", offset);
    }

    pkt->makeAtomicResponse();

    return pioDelay;
}

Tick
MatrixAccelMMIO::write(PacketPtr pkt)
{
    Addr offset = pkt->getAddr() - pioAddr;

    DPRINTF(MatrixAccelMMIO, "write: offset %d\n", offset);
    panic_if(offset >= pioSize, "out of bounds write in %s", name());

    uint32_t write_value = 0;

    if (offset == CONTROL_OFFSET) {
        write_value = pkt->getLE<uint32_t>();
        control = write_value;

        if (control == 1 && status != Status::BUSY) {
            DPRINTF(MatrixAccelMMIO,
                    "write: START received, launching compute\n");
            status = Status::BUSY;
            schedule(compute_event, curTick() + cyclesToTicks(Cycles(500)));
        }

        DPRINTF(MatrixAccelMMIO, "write: control=%d\n", control);
    } else if (offset == BLOCK_SIZE_OFFSET) {
        write_value = pkt->getLE<uint32_t>();
        panic_if(write_value != 16 && write_value != 32 && write_value != 64,
                 "invalid block_size %d in %s", write_value, name());
        block_size = write_value;
        DPRINTF(MatrixAccelMMIO, "write: block_size=%d\n", block_size);
    } else if (offset == DATA_TYPE_OFFSET) {
        write_value = pkt->getLE<uint32_t>();
        panic_if(write_value > 2, "invalid data_type %d in %s", write_value,
                 name());
        data_type = static_cast<DataType>(write_value);
        DPRINTF(MatrixAccelMMIO, "write: data_type=%d\n", write_value);
    } else if (offset >= BUF_A_OFFSET && offset < BUF_B_OFFSET) {
        Addr buf_offset = offset - BUF_A_OFFSET;
        memcpy(buf_a + buf_offset, pkt->getPtr<void>(), pkt->getSize());
        DPRINTF(MatrixAccelMMIO, "write: buf_a offset=%d size=%d\n",
                buf_offset, pkt->getSize());
    } else if (offset >= BUF_B_OFFSET && offset < BUF_C_OFFSET) {
        Addr buf_offset = offset - BUF_B_OFFSET;
        memcpy(buf_b + buf_offset, pkt->getPtr<void>(), pkt->getSize());
        DPRINTF(MatrixAccelMMIO, "write: buf_b offset=%d size=%d\n",
                buf_offset, pkt->getSize());
    } else {
        panic("MatrixAccelMMIO::write: unknown offset %d in %s", offset,
              name());
    }

    pkt->makeAtomicResponse();

    return pioDelay;
}

void
MatrixAccelMMIO::compute()
{
    DPRINTF(MatrixAccelMMIO, "compute: start\n");

    if (data_type == DataType::INT) {
        matrix_mult<int>();
    } else if (data_type == DataType::FLOAT) {
        matrix_mult<float>();
    } else if (data_type == DataType::DOUBLE) {
        matrix_mult<double>();
    }

    schedule(on_done_event, curTick() + cyclesToTicks(Cycles(500)));
}

void
MatrixAccelMMIO::on_done()
{
    DPRINTF(MatrixAccelMMIO, "on_done: computation complete\n");
    status = Status::DONE;
}

} // namespace gem5
