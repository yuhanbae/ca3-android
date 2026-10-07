#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <optional>
#include <deque>
#include <functional>

namespace ca3 {

// J1939 frame structure
struct J1939Frame {
    uint32_t can_id;
    uint8_t data[8];
    uint8_t dlc;
    uint64_t timestamp_ns;
    
    // Extract PGN from CAN ID
    uint32_t getPGN() const {
        uint8_t pf = (can_id >> 16) & 0xFF;
        uint8_t ps = (can_id >> 8) & 0xFF;
        if (pf < 240) {
            // PDU1 format - destination specific
            return (pf << 8) | ps;
        } else {
            // PDU2 format - broadcast
            return (pf << 8);
        }
    }
    
    // Extract priority
    uint8_t getPriority() const {
        return (can_id >> 26) & 0x07;
    }
    
    // Extract source address
    uint8_t getSourceAddress() const {
        return can_id & 0xFF;
    }
    
    // Check if extended frame (29-bit)
    bool isExtended() const {
        return (can_id & 0x80000000) != 0;
    }
};

// J1939 PGN database entry
struct PgnEntry {
    uint32_t pgn;
    std::string name;
    std::string description;
    uint8_t dlc;
    uint16_t transmission_rate_ms; // 0 = on request
};

// Protocol hypothesis for reverse engineering
struct ProtocolHypothesis {
    enum Type {
        UNKNOWN,
        J1939,
        ISO15765,
        KWP2000,
        UDS,
        CUSTOM
    };
    
    Type type = UNKNOWN;
    std::string description;
    double confidence = 0.0;
    std::vector<uint8_t> sample_frame;
    std::string notes;
};

// Main protocol analyzer
class ProtocolAnalyzer {
public:
    struct Config {
        size_t max_frames = 10000;
        bool enable_j1939 = true;
        bool enable_iso15765 = true;
    };
    
    explicit ProtocolAnalyzer(Config config = {});
    
    // Feed raw USB payload (not J1939 frames yet)
    void feedUsbPayload(const uint8_t* data, size_t len, uint64_t timestamp_ns, bool host_to_device);
    
    // Get collected J1939 frames
    const std::deque<J1939Frame>& getJ1939Frames() const;
    
    // Get protocol hypotheses
    const std::vector<ProtocolHypothesis>& getHypotheses() const;
    
    // Try to decode as J1939
    std::optional<J1939Frame> tryDecodeJ1939(const uint8_t* data, size_t len, uint64_t timestamp_ns);
    
    // Try to decode as ISO-TP (ISO 15765-2)
    std::vector<uint8_t> tryDecodeIsoTp(const uint8_t* data, size_t len);
    
    // Clear all state
    void clear();
    
    // Get statistics
    struct Stats {
        size_t usb_packets_received = 0;
        size_t usb_packets_sent = 0;
        size_t j1939_frames_decoded = 0;
        size_t iso_tp_frames_decoded = 0;
    };
    Stats getStats() const;

private:
    Config config_;
    std::deque<J1939Frame> j1939_frames_;
    std::vector<ProtocolHypothesis> hypotheses_;
    Stats stats_;
    
    // ISO-TP reassembly state
    struct IsoTpState {
        std::vector<uint8_t> buffer;
        bool in_progress = false;
        uint32_t expected_len = 0;
        uint8_t seq_num = 0;
        uint64_t first_timestamp = 0;
    };
    IsoTpState iso_tp_state_;
    
    void analyzeUsbPayload(const uint8_t* data, size_t len, uint64_t timestamp_ns, bool host_to_device);
    void generateHypotheses();
    bool looksLikeJ1939(const uint8_t* data, size_t len);
    bool looksLikeIsoTp(const uint8_t* data, size_t len);
};

} // namespace ca3