#include "dev/matrix_accel/matrix_accel.hh"

#include <iostream>
#include "mem/packet_access.hh"
#include "mem/page_table.hh"
#include "sim/process.hh"

namespace gem5
{

MatrixAccel::MatrixAccel(const MatrixAccelParams& p)
: BasicPioDevice(p, p.pio_size),
  system(p.system),
  mem_port(this, system, 0, 0),
  fetch_A_event([this]{fetch_A();}, name()),
  fetch_B_event([this]{fetch_B();}, name()),
  compute_event([this]{compute();}, name()),
  write_C_event([this]{write_C();}, name()),
  on_done_event([this]{on_done();}, name()) {}

Port& MatrixAccel::getPort(const std::string &name, PortID idx) {
    if (name == "mem_port") {
        return mem_port;
    } else {
        return BasicPioDevice::getPort(name, idx);
    }
}

void MatrixAccel::init() {
    panic_if(!mem_port.isConnected(),
             "DMA port of %s is not connected!", name());
    std::cout << "MatrixAccel::init" << std::endl;
    BasicPioDevice::init();
}

Tick MatrixAccel::read(PacketPtr pkt) {
    Addr offset = pkt->getAddr() - pioAddr;

    std::cout << "MatrixAccel::read: reading offset " << offset << std::endl;
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

    std::cout << "MatrixAccel::write: writing offset " << offset << std::endl;
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
            std::cout
            << "MatrixAccel::write: START command received, launching fetch_A"
            << std::endl;
            status = Status::BUSY;
            fetch_A();
        }

        std::cout << "MatrixAccel::write: writing control="
        << control << std::endl;
    } else if (offset == ADDR_A_OFFSET) {
        write_ptr = pkt->getLE<uint64_t>();
        addr_a = write_ptr;
        std::cout << "MatrixAccel::write: writing addr_a="
        << addr_a << std::endl;
    } else if (offset == ADDR_B_OFFSET) {
        write_ptr = pkt->getLE<uint64_t>();
        addr_b = write_ptr;
        std::cout << "MatrixAccel::write: writing addr_b="
        << addr_b << std::endl;
    } else if (offset == ADDR_C_OFFSET) {
        write_ptr = pkt->getLE<uint64_t>();
        addr_c = write_ptr;
        std::cout << "MatrixAccel::write: writing addr_c="
        << addr_c << std::endl;
    } else if (offset == BLOCK_SIZE_OFFSET) {
        write_value = pkt->getLE<uint32_t>();
        panic_if(write_value != 16 && write_value != 32 && write_value != 64,
                 "invalid block_size %d in %s", write_value, name());
        block_size = write_value;
        std::cout << "MatrixAccel::write: writing block_size="
        << block_size << std::endl;
    } else if (offset == DATA_TYPE_OFFSET) {
        write_value = pkt->getLE<uint32_t>();
        panic_if(write_value > 2, "invalid data_type %d in %s",
             write_value, name());
        data_type = static_cast<DataType>(write_value);
        const char* type_name = (data_type == DataType::INT) ? "int" :
                        (data_type == DataType::FLOAT) ? "float" : "double";
        std::cout << "MatrixAccel::write: data_type="
              << write_value << " (" << type_name << ")" << std::endl;;
    } else {
        panic("MatrixAccel::write: unknown offset %d in %s", offset, name());
    }

    pkt->makeAtomicResponse();

    return pioDelay;
}

void MatrixAccel::fetch_A() {
    std::cout << "MatrixAccel::fetch_A" << std::endl;
    panic_if(mem_port.dmaPending(), "DMA already pending in %s", name());

    std::cout << "MatrixAccel::fetch_A: start fetch from: "
    << addr_a << std::endl;

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
    mem_port.dmaAction(
        MemCmd::ReadReq,
        addr_a,
        total_bytes,
        &fetch_B_event,
        buf_a.data(),
        0,
        Request::UNCACHEABLE
    );
}

void MatrixAccel::fetch_B() {
    std::cout << "MatrixAccel::fetch_B" << std::endl;
    panic_if(mem_port.dmaPending(), "DMA already pending in %s", name());

    std::cout << "MatrixAccel::fetch_B: start fetch from: "
    << addr_b << std::endl;

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
    mem_port.dmaAction(
        MemCmd::ReadReq,
        addr_b,
        total_bytes,
        &compute_event,
        buf_b.data(),
        0,
        Request::UNCACHEABLE
    );
}

void MatrixAccel::compute() {
    std::cout << "MatrixAccel::compute: start"
    << std::endl;

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
    std::cout << "MatrixAccel::write_C" << std::endl;
    panic_if(mem_port.dmaPending(), "DMA already pending in %s", name());

    std::cout << "MatrixAccel::write_C: start write to: "
    << addr_c << std::endl;

    size_t elem_size = 0;

    if (data_type == DataType::INT) {
        elem_size = sizeof(int);
    } else if (data_type == DataType::FLOAT) {
        elem_size = sizeof(float);
    } else if (data_type == DataType::DOUBLE) {
        elem_size = sizeof(double);
    }

    size_t total_bytes = block_size * block_size * elem_size;
    mem_port.dmaAction(
        MemCmd::WriteReq,
        addr_c,
        total_bytes,
        &on_done_event,
        buf_c.data(),
        0,
        Request::UNCACHEABLE
    );
}

void MatrixAccel::on_done() {
    std::cout << "MatrixAccel::on_done: result is done" << std::endl;
    status = Status::DONE;
}

} // namespace gem5
