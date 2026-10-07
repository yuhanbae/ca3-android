#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include "../usb/usb_transport.hpp"
#include "../usb/usb_buffer.hpp"

namespace ca3 {

enum class Ca3TransportState {
    DISCONNECTED,
    CONNECTING,
    INTERFACE_CLAIMED,
    ADAPTER_INIT,
    READY,
    ERROR
};

struct Ca3TransportConfig {
    int vid = 0;
    int pid = 0;
    int interfaceNumber = 0;
    int bulkInEndpoint = 0;
    int bulkOutEndpoint = 0;
    int interruptInEndpoint = 0;
    int interruptOutEndpoint = 0;
    int readTimeoutMs = 2000;
    int writeTimeoutMs = 2000;
};

class Ca3Transport {
public:
    using DataCallback = std::function<void(const uint8_t*, size_t)>;
    using StateCallback = std::function<void(Ca3TransportState)>;
    using ErrorCallback = std::function<void(const std::string&)>;

    Ca3Transport();
    ~Ca3Transport();

    // Configure transport parameters
    void setConfig(const Ca3TransportConfig& config);

    // Open connection to CA3 adapter
    bool open();

    // Close connection
    void close();

    // Send raw data to CA3
    bool send(const uint8_t* data, size_t length);

    // Send raw data (vector)
    bool send(const std::vector<uint8_t>& data);

    // Set data received callback
    void setDataCallback(DataCallback callback);

    // Set state change callback
    void setStateCallback(StateCallback callback);

    // Set error callback
    void setErrorCallback(ErrorCallback callback);

    // Get current state
    Ca3TransportState getState() const;

    // Check if connected
    bool isConnected() const;

    // Get last error
    std::string getLastError() const;

    // CA3-specific commands
    bool sendCommand(uint8_t command, const uint8_t* payload = nullptr, size_t payloadLen = 0);
    bool sendCommand(uint8_t command, const std::vector<uint8_t>& payload);

private:
    Ca3TransportConfig config_;
    std::unique_ptr<UsbTransport> usbTransport_;
    UsbBuffer readBuffer_;
    DoubleBuffer doubleBuffer_;

    Ca3TransportState state_ = Ca3TransportState::DISCONNECTED;
    std::string lastError_;

    DataCallback dataCallback_;
    StateCallback stateCallback_;
    ErrorCallback errorCallback_;

    bool isActive_ = false;

    void setState(Ca3TransportState newState);
    void setError(const std::string& error);
    void processIncomingData();
    bool waitForResponse(uint8_t expectedCommand, uint8_t* response, size_t responseLen, int timeoutMs);
};

} // namespace ca3