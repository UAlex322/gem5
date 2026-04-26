#include "dev/matrix_accel/matrix_accel.hh"

#include "arch/riscv/faults.hh"
#include "cpu/base.hh"
#include "debug/MatrixAccel.hh"
#include "mem/packet_access.hh"
#include "mem/page_table.hh"
#include "sim/process.hh"

namespace gem5
{

using namespace RiscvISA;

MatrixAccel::MatrixAccel(const MatrixAccelParams &p)
    : DmaVirtDevice(p),
      system(p.system),
      pioAddr(p.pio_addr),
      pioSize(p.pio_size),
      pioDelay(p.pio_latency),
      fetch_A_event([this] { fetch_A(); }, name()),
      write_C_event([this] { write_C(); }, name())
{}

void
MatrixAccel::init()
{
    DPRINTF(MatrixAccel, "init\n");
    DmaVirtDevice::init();
}

Tick MatrixAccel::read(PacketPtr pkt) {
    Addr offset = pkt->getAddr() - pioAddr;

    DPRINTF(MatrixAccel, "read: offset %d\n", offset);
    panic_if(offset >= pioSize, "out of bounds read in %s", name());

    uint32_t read_value = 0;

    switch(offset) {
        case STATUS_OFFSET:
            read_value = static_cast<uint32_t>(status);
            break;
        default:
            break;
    }

    pkt->setLE<uint32_t>(read_value);
    pkt->makeAtomicResponse();

    return pioDelay;
}

Tick MatrixAccel::write(PacketPtr pkt) {
    Addr offset = pkt->getAddr() - pioAddr;

    DPRINTF(MatrixAccel, "write: offset %d\n", offset);
    panic_if(offset >= pioSize, "out of bounds write in %s", name());

    uint32_t write_value = 0;
    uint64_t write_ptr = 0;

    if (offset == CONTROL_OFFSET) {
        write_value = pkt->getLE<uint32_t>();
        control = write_value;

        if (control == 1 && status != Status::BUSY) {
            panic_if(addr_a == 0, "addr_a not set in %s", name());
            panic_if(addr_b == 0, "addr_b not set in %s", name());
            panic_if(addr_c == 0, "addr_c not set in %s", name());
            DPRINTF(MatrixAccel, "write: START received, launching fetch_A\n");
            status = Status::BUSY;
            schedule(fetch_A_event, curTick());
        }

        DPRINTF(MatrixAccel, "write: control=%d\n", control);
    } else if (offset == ADDR_A_OFFSET) {
        write_ptr = pkt->getLE<uint64_t>();
        addr_a = write_ptr;
        DPRINTF(MatrixAccel, "write: addr_a=0x%x\n", addr_a);
    } else if (offset == ADDR_B_OFFSET) {
        write_ptr = pkt->getLE<uint64_t>();
        addr_b = write_ptr;
        DPRINTF(MatrixAccel, "write: addr_b=0x%x\n", addr_b);
    } else if (offset == ADDR_C_OFFSET) {
        write_ptr = pkt->getLE<uint64_t>();
        addr_c = write_ptr;
        DPRINTF(MatrixAccel, "write: addr_c=0x%x\n", addr_c);
    } else if (offset == BLOCK_SIZE_OFFSET) {
        write_value = pkt->getLE<uint32_t>();
        panic_if(write_value != 16 && write_value != 32 && write_value != 64,
                 "invalid block_size %d in %s", write_value, name());
        block_size = write_value;
        DPRINTF(MatrixAccel, "write: block_size=%d\n", block_size);
    } else if (offset == DATA_TYPE_OFFSET) {
        write_value = pkt->getLE<uint32_t>();
        panic_if(write_value > 2, "invalid data_type %d in %s",
             write_value, name());
        data_type = static_cast<DataType>(write_value);
        DPRINTF(MatrixAccel, "write: data_type=%d\n", write_value);
    } else {
        panic("MatrixAccel::write: unknown offset %d in %s", offset, name());
    }

    pkt->makeAtomicResponse();

    return pioDelay;
}

TranslationGenPtr
MatrixAccel::translate(Addr vaddr, Addr size)
{
    if (!FullSystem) {
        auto process = system->threads[0]->getProcessPtr();
        return process->pTable->translateRange(vaddr, size);
    }
    // FS: адрес физический, identity mapping
    // TODO: реализовать когда будет доступ к кластеру
    panic("MatrixAccel: FS mode not yet supported");
}

AddrRangeList
MatrixAccel::getAddrRanges() const
{
    return {RangeSize(pioAddr, pioSize)};
}

void MatrixAccel::fetch_A() {
    DPRINTF(MatrixAccel, "fetch_A: reading from 0x%x\n", addr_a);

    size_t elem_size = 0;

    if (data_type == DataType::INT) {
        elem_size = sizeof(int);
    } else if (data_type == DataType::FLOAT) {
        elem_size = sizeof(float);
    } else if (data_type == DataType::DOUBLE) {
        elem_size = sizeof(double);
    }

    size_t total_bytes = block_size * block_size * elem_size;
    buf_a.resize(total_bytes);
    auto *cb =
        new DmaVirtCallback<uint64_t>([this](const uint64_t &) { fetch_B(); });
    dmaReadVirt(addr_a, total_bytes, cb, buf_a.data());
}

void MatrixAccel::fetch_B() {
    DPRINTF(MatrixAccel, "fetch_B: reading from 0x%x\n", addr_b);

    size_t elem_size = 0;

    if (data_type == DataType::INT) {
        elem_size = sizeof(int);
    } else if (data_type == DataType::FLOAT) {
        elem_size = sizeof(float);
    } else if (data_type == DataType::DOUBLE) {
        elem_size = sizeof(double);
    }

    size_t total_bytes = block_size * block_size * elem_size;
    buf_b.resize(total_bytes);
    auto *cb =
        new DmaVirtCallback<uint64_t>([this](const uint64_t &) { compute(); });
    dmaReadVirt(addr_b, total_bytes, cb, buf_b.data());
}

void MatrixAccel::compute() {
    DPRINTF(MatrixAccel, "compute: start\n");

    size_t elem_size = 0;

    if (data_type == DataType::INT) {
        elem_size = sizeof(int);
    } else if (data_type == DataType::FLOAT) {
        elem_size = sizeof(float);
    } else if (data_type == DataType::DOUBLE) {
        elem_size = sizeof(double);
    }

    size_t total_bytes = block_size * block_size * elem_size;
    buf_c.resize(total_bytes);

    if (data_type == DataType::INT) {
        matrix_mult<int>(buf_a.data(), buf_b.data(),
         buf_c.data(), block_size);
    } else if (data_type == DataType::FLOAT) {
        matrix_mult<float>(buf_a.data(), buf_b.data(),
         buf_c.data(), block_size);
    } else if (data_type == DataType::DOUBLE) {
        matrix_mult<double>(buf_a.data(), buf_b.data(),
         buf_c.data(), block_size);
    }

    schedule(write_C_event, curTick() + cyclesToTicks(Cycles(500)));
}

void MatrixAccel::write_C() {
    DPRINTF(MatrixAccel, "write_C: writing to 0x%x\n", addr_c);

    size_t elem_size = 0;

    if (data_type == DataType::INT) {
        elem_size = sizeof(int);
    } else if (data_type == DataType::FLOAT) {
        elem_size = sizeof(float);
    } else if (data_type == DataType::DOUBLE) {
        elem_size = sizeof(double);
    }

    size_t total_bytes = block_size * block_size * elem_size;
    auto *cb =
        new DmaVirtCallback<uint64_t>([this](const uint64_t &) { on_done(); });
    dmaWriteVirt(addr_c, total_bytes, cb, buf_c.data());
}

void MatrixAccel::on_done() {
    DPRINTF(MatrixAccel, "on_done: computation complete\n");
    status = Status::DONE;
    if (FullSystem) {
        auto tc = system->threads[0];
        tc->getCpuPtr()->postInterrupt(tc->threadId(),
                                       ExceptionCode::INT_EXT_MACHINE, 0);
    }
}

} // namespace gem5
