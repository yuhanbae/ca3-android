#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>

namespace ca3 {

// CDL (Caterpillar Data Link) Protocol
// Proprietary Caterpillar protocol - reverse engineering scaffold
// NOT IMPLEMENTED - placeholder for future development

struct CdlFrame {
    uint8_t header[4];
    uint8_t data[256];
    size_t length;
    uint64_t timestamp_ns;
    bool valid = false;
};

// CDL message types (hypothesized)
enum class CdlMessageType : uint8_t {
    UNKNOWN = 0,
    REQUEST = 0x01,
    RESPONSE = 0x02,
    BROADCAST = 0x03,
    ACK = 0x04,
    NAK = 0x05,
    HEARTBEAT = 0x06,
    DIAGNOSTIC = 0x10,
    PROGRAMMING = 0x20,
    CALIBRATION = 0x30
};

// CDL address types
struct CdlAddress {
    uint8_t source;
    uint8_t destination;
    uint8_t network;  // Network ID for multi-network systems
};

// CDL decoder interface
class CdlDecoder {
public:
    struct DecodeResult {
        CdlFrame frame;
        CdlMessageType type;
        std::string description;
        double confidence;
    };
    
    virtual ~CdlDecoder() = default;
    
    // Try to decode raw bytes as CDL
    virtual std::optional<DecodeResult> tryDecode(const uint8_t* data, size_t len, uint64_t timestamp_ns) = 0;
    
    // Reset decoder state
    virtual void reset() = 0;
    
    // Get decoder name
    virtual const char* getName() const = 0;
};

// Hypothesis 1: CDL over CAN (J1939-like)
class CdlOverCanDecoder : public CdlDecoder {
public:
    std::optional<DecodeResult> tryDecode(const uint8_t* data, size_t len, uint64_t timestamp_ns) override {
        // Look for CDL-specific CAN IDs
        // Caterpillar often uses specific PGN ranges
        return std::nullopt;
    }
    
    void reset() override {}
    const char* getName() const override { return "CDL-over-CAN"; }
};

// Hypothesis 2: CDL over USB bulk endpoints (raw)
class CdlUsbDecoder : public CdlDecoder {
public:
    std::optional<DecodeResult> tryDecode(const uint8_t* data, size_t len, uint64_t timestamp_ns) override {
        if (len < 4) return std::nullopt;
        
        // Check for known CDL sync patterns
        static const uint8_t cdl_sync[] = {0x55, 0xAA, 0x55, 0xAA};
        static const uint8_t cdl_sync2[] = {0xAA, 0x55, 0xAA, 0x55};
        
        if (std::equal(data, data + 4, std::begin(cdl_sync)) ||
            std::equal(data, data + 4, std::begin(cdl_sync2))) {
            
            DecodeResult result;
            result.frame.timestamp_ns = timestamp_ns;
            result.frame.length = std::min(len, sizeof(result.frame.data));
            std::copy(data, data + result.frame.length, result.frame.data);
            result.type = CdlMessageType::UNKNOWN;
            result.description = "Possible CDL sync pattern detected";
            result.confidence = 0.3;
            return result;
        }
        return std::nullopt;
    }
    
    void reset() override {}
    const char* getName() const override { return "CDL-USB-Raw"; }
};

// CDL analyzer - coordinates multiple decoders
class CdlAnalyzer {
public:
    void addDecoder(std::unique_ptr<CdlDecoder> decoder) {
        decoders_.push_back(std::move(decoder));
    }
    
    std::vector<CdlDecoder::DecodeResult> analyze(const uint8_t* data, size_t len, uint64_t timestamp_ns) {
        std::vector<CdlDecoder::DecodeResult> results;
        for (auto& decoder : decoders_) {
            if (auto result = decoder->tryDecode(data, len, timestamp_ns)) {
                results.push_back(*result);
            }
        }
        // Sort by confidence
        std::sort(results.begin(), results.end(),
            [](const auto& a, const auto& b) { return a.confidence > b.confidence; });
        return results;
    }
    
    void reset() {
        for (auto& decoder : decoders_) {
            decoder->reset();
        }
    }

private:
    std::vector<std::unique_ptr<CdlDecoder>> decoders_;
};

// CDL database placeholder
struct CdlParameter {
    uint16_t pid;           // Parameter ID
    std::string name;
    std::string description;
    std::string units;
    double scale;
    double offset;
    uint8_t data_type;      // 0=uint8, 1=uint16, 2=uint32, 3=int8, 4=int16, 5=int32, 6=float
};

class CdlDatabase {
public:
    static CdlDatabase& instance() {
        static CdlDatabase db;
        return db;
    }
    
    std::optional<CdlParameter> getParameter(uint16_t pid) const {
        auto it = params_.find(pid);
        if (it != params_.end()) return it->second;
        return std::nullopt;
    }
    
    void addParameter(const CdlParameter& param) {
        params_[param.pid] = param;
    }

private:
    std::unordered_map<uint16_t, CdlParameter> params_;
    
    CdlDatabase() {
        // TODO: Add known CDL parameters from reverse engineering
    }
};

} // namespace ca3