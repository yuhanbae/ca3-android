#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <optional>
#include <unordered_map>
#include "../ca3/ca3_transport.hpp"

namespace ca3 {

// CAT (Caterpillar) Protocol Module
// Independent protocol module for Caterpillar diagnostic protocol

struct CatFrame {
    uint8_t serviceId = 0;
    uint16_t parameterId = 0;
    std::vector<uint8_t> data;
    uint64_t timestampNs = 0;
};

enum class CatService : uint8_t {
    START_COMMUNICATION = 0x10,
    STOP_COMMUNICATION = 0x20,
    READ_DATA_BY_IDENTIFIER = 0x22,
    READ_MEMORY_BY_ADDRESS = 0x23,
    READ_SCALING_DATA = 0x24,
    SECURITY_ACCESS = 0x27,
    COMMUNICATION_CONTROL = 0x28,
    READ_DATA_BY_PERIODIC_IDENTIFIER = 0x2A,
    DYNAMICALLY_DEFINE_DATA_IDENTIFIER = 0x2C,
    WRITE_DATA_BY_IDENTIFIER = 0x2E,
    INPUT_OUTPUT_CONTROL_BY_IDENTIFIER = 0x2F,
    ROUTINE_CONTROL = 0x31,
    REQUEST_DOWNLOAD = 0x34,
    REQUEST_UPLOAD = 0x35,
    TRANSFER_DATA = 0x36,
    REQUEST_TRANSFER_EXIT = 0x37,
    WRITE_MEMORY_BY_ADDRESS = 0x3D,
    TESTER_PRESENT = 0x3E,
    ACCESS_TIMING_PARAMETER = 0x83,
    SECURED_DATA_TRANSMISSION = 0x84,
    CONTROL_DTC_SETTING = 0x85,
    RESPONSE_ON_EVENT = 0x86,
    LINK_CONTROL = 0x87
};

enum class CatError : uint8_t {
    POSITIVE_RESPONSE = 0x00,
    GENERAL_REJECT = 0x10,
    SERVICE_NOT_SUPPORTED = 0x11,
    SUBFUNCTION_NOT_SUPPORTED = 0x12,
    INCORRECT_MESSAGE_LENGTH = 0x13,
    RESPONSE_TOO_LONG = 0x14,
    BUSY_REPEAT_REQUEST = 0x21,
    CONDITIONS_NOT_CORRECT = 0x22,
    REQUEST_SEQUENCE_ERROR = 0x24,
    NO_RESPONSE_FROM_SUBCOMPONENT = 0x25,
    FAILURE_PREVENTS_EXECUTION = 0x26,
    REQUEST_OUT_OF_RANGE = 0x31,
    SECURITY_ACCESS_DENIED = 0x33,
    INVALID_KEY = 0x35,
    EXCEEDED_NUMBER_OF_ATTEMPTS = 0x36,
    REQUIRED_TIME_DELAY_NOT_EXPIRED = 0x37,
    UPLOAD_DOWNLOAD_NOT_ACCEPTED = 0x70,
    TRANSFER_DATA_SUSPENDED = 0x71,
    GENERAL_PROGRAMMING_FAILURE = 0x72,
    WRONG_BLOCK_SEQUENCE_COUNTER = 0x73,
    REQUEST_CORRECTLY_RECEIVED_RESPONSE_PENDING = 0x78
};

struct CatParameter {
    uint16_t pid = 0;
    std::string name;
    std::string description;
    std::string units;
    double scale = 1.0;
    double offset = 0.0;
    uint8_t dataLength = 0;

    double decode(const uint8_t* data) const {
        uint64_t raw = 0;
        for (int i = 0; i < dataLength && i < 8; ++i) {
            raw |= (static_cast<uint64_t>(data[i]) << (i * 8));
        }
        return raw * scale + offset;
    }
};

struct CatECU {
    uint8_t ecuId = 0;
    std::string name;
    std::string description;
    std::vector<uint16_t> supportedParameters;
    uint8_t protocolVersion = 0;
};

class CatProtocol {
public:
    using FrameCallback = std::function<void(const CatFrame&)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    using ECUCallback = std::function<void(const CatECU&)>;

    CatProtocol() = default;
    ~CatProtocol() = default;

    void setCa3Transport(std::shared_ptr<Ca3Transport> transport) {
        ca3Transport_ = transport;
    }

    void setFrameCallback(std::function<void(const CatFrame&)> callback) {
        frameCallback_ = std::move(callback);
    }

    void setErrorCallback(std::function<void(const std::string&)> callback) {
        errorCallback_ = std::move(callback);
    }

    void setECUCallback(std::function<void(const CatECU&)> callback) {
        ecuCallback_ = std::move(callback);
    }

    bool connect() {
        if (!ca3Transport_) return false;
        connected_ = ca3Transport_->isConnected();
        if (connected_) {
            discoverECUs();
        }
        return connected_;
    }

    void disconnect() {
        connected_ = false;
        discoveredECUs_.clear();
    }

    bool isConnected() const { return connected_; }

    // Request security access
    bool requestSecurityAccess(uint8_t level) {
        if (!connected_) return false;
        std::vector<uint8_t> payload = {static_cast<uint8_t>(CatService::SECURITY_ACCESS), level};
        return ca3Transport_ && ca3Transport_->sendCommand(0x10, payload);
    }

    // Send security key
    bool sendSecurityKey(uint8_t level, const std::vector<uint8_t>& key) {
        if (!connected_) return false;
        std::vector<uint8_t> payload = {static_cast<uint8_t>(CatService::SECURITY_ACCESS), level};
        payload.insert(payload.end(), key.begin(), key.end());
        return ca3Transport_ && ca3Transport_->sendCommand(0x11, payload);
    }

    // Read data by identifier
    bool readDataByIdentifier(uint16_t parameterId) {
        if (!connected_) return false;
        std::vector<uint8_t> payload = {static_cast<uint8_t>(CatService::READ_DATA_BY_IDENTIFIER),
                                        static_cast<uint8_t>(parameterId & 0xFF),
                                        static_cast<uint8_t>((parameterId >> 8) & 0xFF)};
        return ca3Transport_ && ca3Transport_->sendCommand(0x22, payload);
    }

    // Write data by identifier
    bool writeDataByIdentifier(uint16_t parameterId, const std::vector<uint8_t>& data) {
        if (!connected_) return false;
        std::vector<uint8_t> payload = {static_cast<uint8_t>(CatService::WRITE_DATA_BY_IDENTIFIER),
                                        static_cast<uint8_t>(parameterId & 0xFF),
                                        static_cast<uint8_t>((parameterId >> 8) & 0xFF)};
        payload.insert(payload.end(), data.begin(), data.end());
        return ca3Transport_ && ca3Transport_->sendCommand(0x2E, payload);
    }

    // Tester present
    bool sendTesterPresent(bool suppressResponse = false) {
        if (!connected_) return false;
        std::vector<uint8_t> payload = {static_cast<uint8_t>(CatService::TESTER_PRESENT),
                                        suppressResponse ? 0x80 : 0x00};
        return ca3Transport_ && ca3Transport_->sendCommand(0x3E, payload);
    }

    // Start communication
    bool startCommunication(uint8_t controlType = 0x01) {
        if (!connected_) return false;
        std::vector<uint8_t> payload = {static_cast<uint8_t>(CatService::START_COMMUNICATION), controlType};
        return ca3Transport_ && ca3Transport_->sendCommand(0x10, payload);
    }

    // Stop communication
    bool stopCommunication() {
        if (!connected_) return false;
        std::vector<uint8_t> payload = {static_cast<uint8_t>(CatService::STOP_COMMUNICATION)};
        return ca3Transport_ && ca3Transport_->sendCommand(0x20, payload);
    }

    // Request download (for programming)
    bool requestDownload(uint32_t address, uint32_t size) {
        if (!connected_) return false;
        std::vector<uint8_t> payload = {static_cast<uint8_t>(CatService::REQUEST_DOWNLOAD)};
        // Address (4 bytes, big endian)
        payload.push_back((address >> 24) & 0xFF);
        payload.push_back((address >> 16) & 0xFF);
        payload.push_back((address >> 8) & 0xFF);
        payload.push_back(address & 0xFF);
        // Size (4 bytes, big endian)
        payload.push_back((size >> 24) & 0xFF);
        payload.push_back((size >> 16) & 0xFF);
        payload.push_back((size >> 8) & 0xFF);
        payload.push_back(size & 0xFF);
        return ca3Transport_ && ca3Transport_->sendCommand(0x34, payload);
    }

    // Transfer data
    bool transferData(uint8_t sequenceNumber, const std::vector<uint8_t>& data) {
        if (!connected_) return false;
        std::vector<uint8_t> payload = {static_cast<uint8_t>(CatService::TRANSFER_DATA), sequenceNumber};
        payload.insert(payload.end(), data.begin(), data.end());
        return ca3Transport_ && ca3Transport_->sendCommand(0x36, payload);
    }

    // Request transfer exit
    bool requestTransferExit() {
        if (!connected_) return false;
        std::vector<uint8_t> payload = {static_cast<uint8_t>(CatService::REQUEST_TRANSFER_EXIT)};
        return ca3Transport_ && ca3Transport_->sendCommand(0x37, payload);
    }

    // ECU discovery
    void discoverECUs() {
        // This would send appropriate discovery commands
        // For now, we'll populate with known ECUs
        populateKnownECUs();
    }

    void feedFrame(const CatFrame& frame) {
        if (frameCallback_) frameCallback_(frame);
    }

    void feedRawData(const uint8_t* data, size_t len, uint64_t timestampNs) {
        // Parse CAT frames from raw data
        parseCatData(data, len, timestampNs);
    }

    const std::vector<CatECU>& getDiscoveredECUs() const { return discoveredECUs_; }
    const std::unordered_map<uint16_t, CatParameter>& getParameterDatabase() const { return parameterDatabase_; }

    static const std::unordered_map<uint16_t, CatParameter>& getStaticParameterDatabase();

private:
    std::shared_ptr<Ca3Transport> ca3Transport_;
    bool connected_ = false;
    std::vector<CatECU> discoveredECUs_;
    std::unordered_map<uint16_t, CatParameter> parameterDatabase_;

    std::function<void(const CatFrame&)> frameCallback_;
    std::function<void(const std::string&)> errorCallback_;
    std::function<void(const CatECU&)> ecuCallback_;

    void populateKnownECUs() {
        // Populate with known Caterpillar ECUs
        discoveredECUs_.clear();

        CatECU ecm;
        ecm.ecuId = 0x01;
        ecm.name = "ECM - Engine Control Module";
        ecm.description = "Primary engine control module";
        ecm.protocolVersion = 1;
        ecm.supportedParameters = {0xF004, 0xFEEE, 0xF001, 0x110, 0x190};
        discoveredECUs_.push_back(ecm);

        CatECU tcm;
        tcm.ecuId = 0x02;
        tcm.name = "TCM - Transmission Control Module";
        tcm.description = "Transmission control module";
        tcm.protocolVersion = 1;
        tcm.supportedParameters = {0xFE6F, 0xFECA};
        discoveredECUs_.push_back(tcm);

        CatECU bcm;
        bcm.ecuId = 0x03;
        bcm.name = "BCM - Brake Control Module";
        bcm.description = "Brake system control module";
        bcm.protocolVersion = 1;
        bcm.supportedParameters = {0xF001};
        discoveredECUs_.push_back(bcm);

        for (const auto& ecu : discoveredECUs_) {
            if (ecuCallback_) ecuCallback_(ecu);
        }
    }

    void parseCatData(const uint8_t* data, size_t len, uint64_t timestampNs) {
        if (len < 2) return;

        CatFrame frame;
        frame.serviceId = data[0];
        frame.timestampNs = timestampNs;

        if (len >= 3) {
            frame.parameterId = data[1] | (data[2] << 8);
            if (len > 3) {
                frame.data.assign(data + 3, data + len);
            }
        }

        if (frameCallback_) frameCallback_(frame);
    }
};

// Static parameter database
std::unordered_map<uint16_t, CatParameter> CatProtocol::parameterDatabase_ = {
    // Engine parameters
    {0xF004, {0xF004, "Engine Speed", "Engine speed", "rpm", 0.125, 0.0, 2}},
    {0x005C, {0x005C, "Engine Speed (alt)", "Engine speed", "rpm", 1.0, 0.0, 2}},
    {0x005D, {0x005D, "Engine Coolant Temperature", "Engine coolant temperature", "°C", 1.0, -40.0, 1}},
    {0x005E, {0x005E, "Engine Oil Pressure", "Engine oil pressure", "kPa", 4.0, 0.0, 1}},
    {0x005F, {0x005F, "Engine Oil Temperature", "Engine oil temperature", "°C", 1.0, -40.0, 1}},
    {0x0060, {0x0060, "Fuel Pressure", "Fuel pressure", "kPa", 4.0, 0.0, 1}},
    {0x0061, {0x0061, "Boost Pressure", "Boost pressure", "kPa", 4.0, 0.0, 1}},
    {0x0062, {0x0062, "Intake Manifold Temperature", "Intake manifold temperature", "°C", 1.0, -40.0, 1}},
    {0x0063, {0x0063, "Exhaust Temperature", "Exhaust temperature", "°C", 1.0, -40.0, 2}},
    {0x0064, {0x0064, "Fuel Rate", "Fuel rate", "L/h", 0.05, 0.0, 2}},
    {0x0065, {0x0065, "Instantaneous Fuel Economy", "Fuel economy", "km/L", 0.002, 0.0, 2}},
    {0x0066, {0x0066, "Trip Fuel Used", "Trip fuel used", "L", 0.5, 0.0, 2}},
    {0x0067, {0x0067, "Total Fuel Used", "Total fuel used", "L", 0.5, 0.0, 3}},
    {0x0068, {0x0068, "Vehicle Speed", "Vehicle speed", "km/h", 1.0/256.0, 0.0, 2}},
    {0x0069, {0x0069, "Trip Distance", "Trip distance", "km", 0.125, 0.0, 3}},
    {0x006A, {0x006A, "Total Vehicle Distance", "Total vehicle distance", "km", 0.125, 0.0, 4}},
    {0x006B, {0x006B, "Battery Voltage", "Battery voltage", "V", 0.05, 0.0, 1}},
    {0x006C, {0x006C, "Alternator Voltage", "Alternator voltage", "V", 0.05, 0.0, 1}},
    {0x006D, {0x006D, "Transmission Oil Temperature", "Transmission oil temperature", "°C", 1.0, -40.0, 1}},
    {0x006E, {0x006E, "Current Gear", "Current gear", "", 1.0, 0.0, 1}},
    {0x006F, {0x006F, "Brake Application Pressure", "Brake application pressure", "kPa", 4.0, 0.0, 1}},
    {0x0070, {0x0070, "Primary Reservoir Pressure", "Primary air reservoir pressure", "kPa", 4.0, 0.0, 1}},
    {0x0071, {0x0071, "Secondary Reservoir Pressure", "Secondary air reservoir pressure", "kPa", 4.0, 0.0, 1}},
    {0x0072, {0x0072, "Active Diagnostic Codes", "Active diagnostic codes", "", 1.0, 0.0, 2}},
    {0x0073, {0x0073, "Previous Diagnostic Codes", "Previous diagnostic codes", "", 1.0, 0.0, 2}},
    {0x0074, {0x0074, "Clear Diagnostic Codes", "Clear diagnostic codes", "", 1.0, 0.0, 1}},

    // J1939 PGNs as parameters
    {0xF004, {0xF004, "EEC1 - Engine Speed", "Engine speed", "rpm", 0.125, 0.0, 2}},
    {0xFEEE, {0xFEEE, "Engine Temperature", "Engine temperature", "°C", 1.0, -40.0, 2}},
    {0xFECA, {0xFECA, "Tachograph", "Tachograph", "h", 0.05, 0.0, 3}},
    {0xFE6F, {0xFE6F, "Dash Display", "Dash display", "km/h", 0.00390625, 0.0, 2}},
    {0xF001, {0xF001, "EBC1 - Brake", "Electronic brake", "", 1.0, 0.0, 2}},
};

const std::unordered_map<uint16_t, CatParameter>& CatProtocol::getStaticParameterDatabase() {
    return parameterDatabase_;
}

void CatProtocol::populateKnownECUs() {
    // Implementation in header
}

void CatProtocol::parseCatData(const uint8_t* data, size_t len, uint64_t timestampNs) {
    // Implementation in header
}

const std::unordered_map<uint16_t, CatParameter>& CatProtocol::getStaticParameterDatabase() {
    return parameterDatabase_;
}

} // namespace ca3