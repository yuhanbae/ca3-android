#include "usb_transport.hpp"
#include <android/log.h>

#define LOG_TAG "CA3UsbTransport"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace ca3 {

UsbTransport::UsbTransport() = default;

UsbTransport::~UsbTransport() {
    close();
}

bool UsbTransport::open(int vid, int pid, int interfaceNumber, int bulkInEp, int bulkOutEp, int interruptInEp, int interruptOutEp) {
    if (isOpen_) return true;

    // Store configuration
    vid_ = vid;
    pid_ = pid;
    interfaceNumber_ = interfaceNumber;
    bulkInEndpoint_ = bulkInEp;
    bulkOutEndpoint_ = bulkOutEp;
    interruptInEndpoint_ = interruptInEp;
    interruptOutEndpoint_ = interruptOutEp;

    LOGI("Opening USB transport: VID=0x%04X PID=0x%04X Interface=%d", vid, pid, interfaceNumber);

    // Note: Actual USB device opening is done via JNI to Android UsbManager
    // This C++ layer manages the transport protocol state

    isOpen_ = true;
    return true;
}

void UsbTransport::close() {
    if (isOpen_) {
        isOpen_ = false;
        readBuffer_.clear();
        LOGI("USB transport closed");
    }
}

int UsbTransport::read(uint8_t* buffer, size_t capacity, int timeoutMs) {
    if (!isOpen_) return -1;
    if (capacity == 0) return 0;

    // In a real implementation, this would read from the USB bulk IN endpoint
    // via JNI to Android UsbDeviceConnection.bulkTransfer()

    // For now, return 0 (no data available)
    return 0;
}

int UsbTransport::write(const uint8_t* buffer, size_t length, int timeoutMs) {
    if (!isOpen_) return -1;
    if (length == 0) return 0;

    // In a real implementation, this would write to the USB bulk OUT endpoint
    // via JNI to Android UsbDeviceConnection.bulkTransfer()

    // For now, return length as if written
    return static_cast<int>(length);
}

int UsbTransport::controlTransfer(
    uint8_t requestType,
    uint8_t request,
    uint16_t value,
    uint16_t index,
    uint8_t* buffer,
    uint16_t length,
    int timeoutMs
) {
    if (!isOpen_) return -1;

    // In a real implementation, this would perform a USB control transfer
    // via JNI to Android UsbDeviceConnection.controlTransfer()

    return 0;
}

bool UsbTransport::isConnected() const {
    return isOpen_;
}

void UsbTransport::flushReadBuffer() {
    readBuffer_.clear();
}

size_t UsbTransport::getReadBufferSize() const {
    return readBuffer_.size();
}

} // namespace ca3