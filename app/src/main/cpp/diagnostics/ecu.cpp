#include "diagnostic_session.hpp"
#include <android/log.h>

#define LOG_TAG "DiagnosticECU"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace ca3 {

// ECU identification and management

struct EcuIdentification {
    uint8_t ecuId = 0;
    std::string name;
    std::string partNumber;
    std::string softwareVersion;
    std::string hardwareVersion;
    std::string vin;
    uint8_t protocolVersion = 0;
    std::string manufacturer;
    std::string systemName;
};

// Known ECU database for Caterpillar
static const std::unordered_map<uint8_t, EcuIdentification> knownECUs = {
    {0x01, {0x01, "ECM - Engine Control Module", "", "", "", "", 1, "Caterpillar", "Engine"}},
    {0x02, {0x02, "TCM - Transmission Control Module", "", "", "", "", 1, "Caterpillar", "Transmission"}},
    {0x03, {0x03, "BCM - Brake Control Module", "", "", "", "", 1, "Caterpillar", "Brakes"}},
    {0x04, {0x04, "ICM - Instrument Cluster Module", "", "", "", "", 1, "Caterpillar", "Instrument"}},
    {0x05, {0x05, "HCM - Hydraulic Control Module", "", "", "", "", 1, "Caterpillar", "Hydraulics"}},
    {0x06, {0x06, "SCM - Steering Control Module", "", "", "", "", 1, "Caterpillar", "Steering"}},
    {0x07, {0x07, "RCM - Restraint Control Module", "", "", "", "", 1, "Caterpillar", "Safety"}},
    {0x08, {0x08, "DCM - Door Control Module", "", "", "", "", 1, "Caterpillar", "Body"}},
    {0x09, {0x09, "ACM - Air Conditioning Module", "", "", "", "", 1, "Caterpillar", "HVAC"}},
    {0x0A, {0x0A, "FCM - Fuel Control Module", "", "", "", "", 1, "Caterpillar", "Fuel"}},
    {0x0B, {0x0B, "ECM2 - Secondary Engine Control", "", "", "", "", 1, "Caterpillar", "Engine"}},
    {0x0C, {0x0C, "TCM2 - Secondary Transmission", "", "", "", "", 1, "Caterpillar", "Transmission"}},
    {0x10, {0x10, "J1939 Gateway", "", "", "", "", 1, "Caterpillar", "Gateway"}},
    {0x11, {0x11, "Telematics Gateway", "", "", "", "", 1, "Caterpillar", "Telematics"}},
    {0x12, {0x12, "Diagnostic Gateway", "", "", "", "", 1, "Caterpillar", "Diagnostics"}},
    {0xF0, {0xF0, "Bootloader", "", "", "", "", 1, "Caterpillar", "System"}},
    {0xFF, {0xFF, "Broadcast/All", "", "", "", "", 1, "Caterpillar", "All"}}
};

EcuIdentification identifyECU(uint8_t ecuId) {
    auto it = knownECUs.find(ecuId);
    if (it != knownECUs.end()) {
        return it->second;
    }
    return {ecuId, "Unknown ECU (0x" + std::to_string(ecuId) + ")", "", "", "", "", 0, "", ""};
}

std::vector<EcuIdentification> getAllKnownECUs() {
    std::vector<EcuIdentification> result;
    for (const auto& pair : knownECUs) {
        result.push_back(pair.second);
    }
    return result;
}

bool isValidECUId(uint8_t ecuId) {
    return knownECUs.find(ecuId) != knownECUs.end();
}

std::string getECUName(uint8_t ecuId) {
    auto it = knownECUs.find(ecuId);
    if (it != knownECUs.end()) {
        return it->second.name;
    }
    return "Unknown ECU (0x" + std::to_string(ecuId) + ")";
}

std::string getECUSystem(uint8_t ecuId) {
    auto it = knownECUs.find(ecuId);
    if (it != knownECUs.end()) {
        return it->second.systemName;
    }
    return "Unknown";
}

} // namespace ca3