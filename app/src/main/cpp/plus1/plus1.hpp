#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <optional>
#include "../ca3/ca3_transport.hpp"

namespace ca3 {

// PLUS+1 (Danfoss) Protocol Module
// Independent protocol module for Danfoss PLUS+1 controllers

struct Plus1Frame {
    uint8_t serviceId = 0;
    uint8_t nodeId = 0;
    uint16_t index = 0;
    uint8_t subIndex = 0;
    std::vector<uint8_t> data;
    uint64_t timestampNs = 0;
};

enum class Plus1Service : uint8_t {
    READ_REQUEST = 0x40,
    READ_RESPONSE = 0x4B,
    WRITE_REQUEST = 0x2B,
    WRITE_RESPONSE = 0x60,
    ERROR_RESPONSE = 0x80,
    IDENTITY_REQUEST = 0x01,
    IDENTITY_RESPONSE = 0x01
};

enum class Plus1Error : uint32_t {
    NONE = 0,
    TOGGLE_BIT_NOT_ALTERNATED = 0x05030000,
    SDO_PROTOCOL_TIMED_OUT = 0x05040000,
    CLIENT_SERVER_COMMAND_SPECIFIER_NOT_VALID = 0x05040001,
    INVALID_BLOCK_SIZE = 0x05040002,
    INVALID_SEQUENCE_NUMBER = 0x05040003,
    CRC_ERROR = 0x05040004,
    OUT_OF_MEMORY = 0x05040005,
    UNSUPPORTED_ACCESS = 0x06010000,
    ATTEMPT_TO_READ_WRITE_ONLY = 0x06010001,
    ATTEMPT_TO_READ_WRITE_ONLY_2 = 0x06010002,
    OBJECT_NOT_IN_DICTIONARY = 0x06020000,
    OBJECT_CANNOT_BE_MAPPED = 0x06040041,
    MAPPED_OBJECTS_EXCEED_PDO_LENGTH = 0x06040042,
    GENERAL_PARAMETER_INCOMPATIBILITY = 0x06040043,
    GENERAL_INTERNAL_INCOMPATIBILITY = 0x06040047,
    HARDWARE_ERROR = 0x06060000,
    TYPE_MISMATCH = 0x06070010,
    DATA_TOO_LONG = 0x06070012,
    DATA_TOO_SHORT = 0x06070013,
    SUBINDEX_NOT_EXIST = 0x06090011,
    INVALID_VALUE = 0x06090030,
    VALUE_TOO_HIGH = 0x06090031,
    VALUE_TOO_LOW = 0x06090032,
    MAXIMUM_LESS_THAN_MINIMUM = 0x06090036,
    GENERAL_ERROR = 0x08000000,
    DATA_TRANSFER_ERROR = 0x08000020,
    LOCAL_CONTROL = 0x08000021,
    DEVICE_STATE = 0x08000022
};

class Plus1Protocol {
public:
    using FrameCallback = std::function<void(const Plus1Frame&)>;
    using ErrorCallback = std::function<void(Plus1Error, const std::string&)>;

    Plus1Protocol() = default;
    ~Plus1Protocol() = default;

    void setCa3Transport(std::shared_ptr<Ca3Transport> transport) {
        ca3Transport_ = transport;
    }

    void setFrameCallback(std::function<void(const Plus1Frame&)> callback) {
        frameCallback_ = std::move(callback);
    }

    void setErrorCallback(std::function<void(Plus1Error, const std::string&)> callback) {
        errorCallback_ = std::move(callback);
    }

    bool connect() {
        if (!ca3Transport_) return false;
        connected_ = ca3Transport_->isConnected();
        return connected_;
    }

    void disconnect() {
        connected_ = false;
    }

    bool isConnected() const { return connected_; }

    // Read object from dictionary
    bool readObject(uint8_t nodeId, uint16_t index, uint8_t subIndex) {
        if (!connected_) return false;
        // Build SDO read request
        std::vector<uint8_t> payload = {
            static_cast<uint8_t>(Plus1Service::READ_REQUEST),
            static_cast<uint8_t>(index & 0xFF),
            static_cast<uint8_t>((index >> 8) & 0xFF),
            subIndex
        };
        // Send via CA3 transport
        return ca3Transport_ && ca3Transport_->sendCommand(0x01, payload);
    }

    // Write object to dictionary
    bool writeObject(uint8_t nodeId, uint16_t index, uint8_t subIndex, const std::vector<uint8_t>& data) {
        if (!connected_) return false;
        // Build SDO write request
        return ca3Transport_ && ca3Transport_->sendCommand(0x02, data);
    }

    // Request node identity
    bool requestIdentity(uint8_t nodeId) {
        if (!connected_) return false;
        std::vector<uint8_t> payload = {
            static_cast<uint8_t>(Plus1Service::IDENTITY_REQUEST)
        };
        return ca3Transport_ && ca3Transport_->sendCommand(0x03, payload);
    }

    void feedFrame(const Plus1Frame& frame) {
        if (frameCallback_) frameCallback_(frame);
    }

    void feedRawData(const uint8_t* data, size_t len, uint64_t timestampNs) {
        // Parse PLUS+1 frames from raw data
        // Implementation depends on actual PLUS+1 encapsulation
    }

private:
    std::shared_ptr<Ca3Transport> ca3Transport_;
    bool connected_ = false;
    std::function<void(const Plus1Frame&)> frameCallback_;
    std::function<void(Plus1Error, const std::string&)> errorCallback_;
};

// PLUS+1 Object Dictionary common entries
namespace Plus1OD {
    // Communication profile area (0x1000-0x1FFF)
    constexpr uint16_t DEVICE_TYPE = 0x1000;
    constexpr uint16_t ERROR_REGISTER = 0x1001;
    constexpr uint16_t MANUFACTURER_DEVICE_NAME = 0x1008;
    constexpr uint16_t MANUFACTURER_HARDWARE_VERSION = 0x1009;
    constexpr uint16_t MANUFACTURER_SOFTWARE_VERSION = 0x100A;
    constexpr uint16_t IDENTITY_OBJECT = 0x1018;

    // Manufacturer specific area (0x2000-0x5FFF)
    // Danfoss specific parameters would be here

    // Standardized device profile area (0x6000-0x9FFF)
    // Application specific parameters
} // namespace Plus1OD

} // namespace ca3