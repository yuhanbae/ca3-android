#pragma once

#include <cstdint>
#include <vector>
#include <memory>

namespace ca3 {

class UsbTransport {
public:
    UsbTransport();
    virtual ~UsbTransport();

    // Open connection to USB device
    // vid/pid: vendor/product ID
    // interfaceNumber: USB interface to claim
    // bulkInEp/bulkOutEp: bulk endpoint addresses (e.g., 0x81, 0x02)
    // interruptInEp/interruptOutEp: interrupt endpoint addresses (optional, 0 if not used)
    virtual bool open(int vid, int pid, int interfaceNumber,
                     int bulkInEp, int bulkOutEp,
                     int interruptInEp = 0, int interruptOutEp = 0);

    virtual void close();

    // Read data from bulk IN endpoint
    // Returns number of bytes read, or -1 on error
    virtual int read(uint8_t* buffer, size_t capacity, int timeoutMs = 2000);

    // Write data to bulk OUT endpoint
    // Returns number of bytes written, or -1 on error
    virtual int write(const uint8_t* buffer, size_t length, int timeoutMs = 2000);

    // USB control transfer
    // Returns number of bytes transferred, or -1 on error
    virtual int controlTransfer(
        uint8_t requestType,
        uint8_t request,
        uint16_t value,
        uint16_t index,
        uint8_t* buffer,
        uint16_t length,
        int timeoutMs = 2000
    );

    virtual bool isConnected() const;

    void flushReadBuffer();
    size_t getReadBufferSize() const;

protected:
    bool isOpen_ = false;
    int vid_ = 0;
    int pid_ = 0;
    int interfaceNumber_ = -1;
    int bulkInEndpoint_ = 0;
    int bulkOutEndpoint_ = 0;
    int interruptInEndpoint_ = 0;
    int interruptOutEndpoint_ = 0;
    std::vector<uint8_t> readBuffer_;
};

} // namespace ca3