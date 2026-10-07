#include "ca3_transport.hpp"
#include <android/log.h>
#include <chrono>
#include <thread>

#define LOG_TAG "CA3Transport"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace ca3 {

Ca3Transport::Ca3Transport()
    : usbTransport_(std::make_unique<UsbTransport>()) {}

Ca3Transport::~Ca3Transport() {
    close();
}

void Ca3Transport::setConfig(const Ca3TransportConfig& config) {
    config_ = config;
}

bool Ca3Transport::open() {
    if (state_ != Ca3TransportState::DISCONNECTED) {
        setError("Transport already open");
        return false;
    }

    setState(Ca3TransportState::CONNECTING);

    bool ok = usbTransport_->open(
        config_.vid,
        config_.pid,
        config_.interfaceNumber,
        config_.bulkInEndpoint,
        config_.bulkOutEndpoint,
        config_.interruptInEndpoint,
        config_.interruptOutEndpoint
    );

    if (!ok) {
        setError("Failed to open USB transport");
        setState(Ca3TransportState::ERROR);
        return false;
    }

    setState(Ca3TransportState::INTERFACE_CLAIMED);
    setState(Ca3TransportState::ADAPTER_INIT);

    // Initialize CA3 adapter - send initialization sequence
    // This would be protocol-specific initialization
    std::vector<uint8_t> initCmd = {0x00}; // Placeholder
    if (!send(initCmd.data(), initCmd.size())) {
        setError("Failed to initialize CA3 adapter");
        setState(Ca3TransportState::ERROR);
        return false;
    }

    setState(Ca3TransportState::READY);
    isActive_ = true;
    return true;
}

void Ca3Transport::close() {
    isActive_ = false;
    usbTransport_->close();
    readBuffer_.clear();
    setState(Ca3TransportState::DISCONNECTED);
}

bool Ca3Transport::send(const uint8_t* data, size_t length) {
    if (!isConnected()) return false;

    int written = usbTransport_->write(data, length, config_.writeTimeoutMs);
    if (written != static_cast<int>(length)) {
        setError("USB write failed: " + std::to_string(written) + "/" + std::to_string(length));
        return false;
    }
    return true;
}

bool Ca3Transport::send(const std::vector<uint8_t>& data) {
    return send(data.data(), data.size());
}

void Ca3Transport::setDataCallback(DataCallback callback) {
    dataCallback_ = std::move(callback);
}

void Ca3Transport::setStateCallback(StateCallback callback) {
    stateCallback_ = std::move(callback);
}

void Ca3Transport::setErrorCallback(ErrorCallback callback) {
    errorCallback_ = std::move(callback);
}

Ca3TransportState Ca3Transport::getState() const {
    return state_;
}

bool Ca3Transport::isConnected() const {
    return state_ == Ca3TransportState::READY && usbTransport_->isConnected();
}

std::string Ca3Transport::getLastError() const {
    return lastError_;
}

bool Ca3Transport::sendCommand(uint8_t command, const uint8_t* payload, size_t payloadLen) {
    // Build CA3 command frame
    // Frame format would depend on the actual CA3 protocol
    // This is a placeholder implementation

    std::vector<uint8_t> frame;
    frame.push_back(command);
    if (payload && payloadLen > 0) {
        frame.insert(frame.end(), payload, payload + payloadLen);
    }

    return send(frame);
}

bool Ca3Transport::sendCommand(uint8_t command, const std::vector<uint8_t>& payload) {
    return sendCommand(command, payload.data(), payload.size());
}

void Ca3Transport::setState(Ca3TransportState newState) {
    if (state_ != newState) {
        state_ = newState;
        LOGI("State changed: %d", static_cast<int>(state_));
        if (stateCallback_) {
            stateCallback_(state_);
        }
    }
}

void Ca3Transport::setError(const std::string& error) {
    lastError_ = error;
    LOGE("Error: %s", error.c_str());
    if (errorCallback_) {
        errorCallback_(error);
    }
}

void Ca3Transport::processIncomingData() {
    // Read from USB
    uint8_t tempBuffer[4096];
    int read = usbTransport_->read(tempBuffer, sizeof(tempBuffer), config_.readTimeoutMs);

    if (read > 0) {
        readBuffer_.write(tempBuffer, read);

        // Process complete frames
        while (true) {
            // Try to find complete frame in buffer
            // This would depend on the CA3 frame format
            // For now, just pass all data to callback
            size_t available = readBuffer_.size();
            if (available > 0 && dataCallback_) {
                std::vector<uint8_t> data(available);
                readBuffer_.read(data.data(), available);
                dataCallback_(data.data(), data.size());
            } else {
                break;
            }
        }
    } else if (read < 0) {
        setError("USB read error");
        setState(Ca3TransportState::ERROR);
    }
}

bool Ca3Transport::waitForResponse(uint8_t expectedCommand, uint8_t* response, size_t responseLen, int timeoutMs) {
    // Placeholder - would implement response waiting with timeout
    (void)expectedCommand;
    (void)response;
    (void)responseLen;
    (void)timeoutMs;
    return false;
}

} // namespace ca3