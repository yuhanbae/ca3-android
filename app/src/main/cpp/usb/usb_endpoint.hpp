#pragma once

#include <cstdint>
#include <string>

namespace ca3 {

struct UsbEndpoint {
    uint8_t address = 0;        // Endpoint address (e.g., 0x81, 0x02)
    uint8_t number = 0;         // Endpoint number (0-15)
    std::string direction;      // "IN" or "OUT"
    std::string transferType;   // "CONTROL", "ISOCHRONOUS", "BULK", "INTERRUPT"
    uint16_t maxPacketSize = 0;
    uint8_t interval = 0;

    bool isIn() const { return direction == "IN"; }
    bool isOut() const { return direction == "OUT"; }
    bool isBulk() const { return transferType == "BULK"; }
    bool isInterrupt() const { return transferType == "INTERRUPT"; }
    bool isControl() const { return transferType == "CONTROL"; }
    bool isIsochronous() const { return transferType == "ISOCHRONOUS"; }
};

class UsbEndpointManager {
public:
    static constexpr size_t MAX_ENDPOINTS = 32;

    void addEndpoint(const UsbEndpoint& endpoint) {
        if (endpoints_.size() < MAX_ENDPOINTS) {
            endpoints_.push_back(endpoint);
        }
    }

    const UsbEndpoint* findEndpoint(uint8_t address) const {
        for (const auto& ep : endpoints_) {
            if (ep.address == address) return &ep;
        }
        return nullptr;
    }

    const UsbEndpoint* findBulkIn() const {
        for (const auto& ep : endpoints_) {
            if (ep.isIn() && ep.isBulk()) return &ep;
        }
        return nullptr;
    }

    const UsbEndpoint* findBulkOut() const {
        for (const auto& ep : endpoints_) {
            if (ep.isOut() && ep.isBulk()) return &ep;
        }
        return nullptr;
    }

    const UsbEndpoint* findInterruptIn() const {
        for (const auto& ep : endpoints_) {
            if (ep.isIn() && ep.isInterrupt()) return &ep;
        }
        return nullptr;
    }

    const UsbEndpoint* findInterruptOut() const {
        for (const auto& ep : endpoints_) {
            if (ep.isOut() && ep.isInterrupt()) return &ep;
        }
        return nullptr;
    }

    const std::vector<UsbEndpoint>& getAllEndpoints() const { return endpoints_; }

    void clear() { endpoints_.clear(); }
    size_t count() const { return endpoints_.size(); }

private:
    std::vector<UsbEndpoint> endpoints_;
};

} // namespace ca3