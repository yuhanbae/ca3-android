#include "ca3_protocol.hpp"
#include "ca3_frame.hpp"
#include <algorithm>
#include <cstring>

namespace ca3 {

ProtocolAnalyzer::ProtocolAnalyzer(Config config) : config_(config) {
        // std::deque does not have reserve(), only std::vector does
    hypotheses_.reserve(100);
}

void ProtocolAnalyzer::feedUsbPayload(const uint8_t* data, size_t len, uint64_t timestamp_ns, bool host_to_device) {
    if (host_to_device) {
        stats_.usb_packets_sent++;
    } else {
        stats_.usb_packets_received++;
    }
    
    analyzeUsbPayload(data, len, timestamp_ns, host_to_device);
    
    // Periodically generate hypotheses
    if ((stats_.usb_packets_received + stats_.usb_packets_sent) % 100 == 0) {
        generateHypotheses();
    }
}

void ProtocolAnalyzer::analyzeUsbPayload(const uint8_t* data, size_t len, uint64_t timestamp_ns, bool host_to_device) {
    // Try J1939 decoding
    if (config_.enable_j1939 && looksLikeJ1939(data, len)) {
        auto frame = tryDecodeJ1939(data, len, timestamp_ns);
        if (frame) {
            j1939_frames_.push_back(*frame);
            stats_.j1939_frames_decoded++;
            if (j1939_frames_.size() > config_.max_frames) {
                j1939_frames_.pop_front();
            }
        }
    }
    
    // Try ISO-TP decoding
    if (config_.enable_iso15765 && looksLikeIsoTp(data, len)) {
        auto decoded = tryDecodeIsoTp(data, len);
        if (!decoded.empty()) {
            stats_.iso_tp_frames_decoded++;
        }
    }
}

std::optional<J1939Frame> ProtocolAnalyzer::tryDecodeJ1939(const uint8_t* data, size_t len, uint64_t timestamp_ns) {
    // CA3 may encapsulate J1939 in various ways
    // Hypothesis 1: Raw J1939 frame (4 byte CAN ID + DLC + 8 bytes data)
    if (len == 13) {
        uint32_t can_id = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
        uint8_t dlc = data[4];
        if (dlc <= 8) {
            J1939Frame frame;
            frame.can_id = can_id | 0x80000000; // Extended frame
            frame.dlc = dlc;
            frame.timestamp_ns = timestamp_ns;
            std::memcpy(frame.data, data + 5, std::min<size_t>(dlc, 8));
            return frame;
        }
    }
    
    // Hypothesis 2: J1939 with 3-byte header (PGN + SA)
    if (len >= 11) {
        uint32_t pgn = data[0] | (data[1] << 8) | (data[2] << 16);
        uint8_t sa = data[3];
        uint8_t dlc = data[4];
        if (dlc <= 8 && len >= 5 + dlc) {
            J1939Frame frame;
            frame.can_id = (pgn << 8) | sa | 0x80000000;
            frame.dlc = dlc;
            frame.timestamp_ns = timestamp_ns;
            std::memcpy(frame.data, data + 5, dlc);
            return frame;
        }
    }
    
    // Hypothesis 3: CAN 2.0A/B (11-bit ID)
    if (len >= 10) {
        uint16_t can_id = data[0] | (data[1] << 8);
        uint8_t dlc = data[2];
        if (dlc <= 8 && len >= 3 + dlc) {
            J1939Frame frame;
            frame.can_id = can_id;
            frame.dlc = dlc;
            frame.timestamp_ns = timestamp_ns;
            std::memcpy(frame.data, data + 3, dlc);
            return frame;
        }
    }
    
    return std::nullopt;
}

std::vector<uint8_t> ProtocolAnalyzer::tryDecodeIsoTp(const uint8_t* data, size_t len) {
    if (len < 1) return {};
    
    uint8_t pci_type = data[0] >> 4;
    
    switch (pci_type) {
        case 0: // Single Frame (SF)
            if (len >= 2) {
                uint8_t dlc = data[0] & 0x0F;
                if (dlc <= 7 && len >= 1 + dlc) {
                    std::vector<uint8_t> result(data + 1, data + 1 + dlc);
                    iso_tp_state_ = {};
                    return result;
                }
            }
            break;
            
        case 1: // First Frame (FF)
            if (len >= 3) {
                uint16_t total_len = ((data[0] & 0x0F) << 8) | data[1];
                iso_tp_state_.buffer.clear();
                iso_tp_state_.buffer.reserve(total_len);
                iso_tp_state_.in_progress = true;
                iso_tp_state_.expected_len = total_len;
                iso_tp_state_.seq_num = 0;
                iso_tp_state_.first_timestamp = 0;
                
                // Copy first frame data
                size_t first_data = std::min<size_t>(len - 2, 6);
                iso_tp_state_.buffer.insert(iso_tp_state_.buffer.end(), data + 2, data + 2 + first_data);
            }
            break;
            
        case 2: // Consecutive Frame (CF)
            if (iso_tp_state_.in_progress && len >= 2) {
                uint8_t seq = data[0] & 0x0F;
                if (seq == ((iso_tp_state_.seq_num + 1) & 0x0F)) {
                    iso_tp_state_.buffer.insert(iso_tp_state_.buffer.end(), data + 1, data + len);
                    iso_tp_state_.seq_num = seq;
                    
                    if (iso_tp_state_.buffer.size() >= iso_tp_state_.expected_len) {
                        std::vector<uint8_t> result = iso_tp_state_.buffer;
                        result.resize(iso_tp_state_.expected_len);
                        iso_tp_state_ = {};
                        return result;
                    }
                } else {
                    // Sequence error
                    iso_tp_state_ = {};
                }
            }
            break;
            
        case 3: // Flow Control (FC)
            // Ignore for now
            break;
    }
    
    return {};
}

bool ProtocolAnalyzer::looksLikeJ1939(const uint8_t* data, size_t len) {
    if (len < 4) return false;
    
    // Check for extended CAN ID pattern (bit 29 set)
    uint32_t can_id = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    if (can_id & 0x80000000) {
        // Check DLC validity
        if (len >= 5 && data[4] <= 8) {
            return true;
        }
    }
    
    // Check for 3-byte PGN + SA + DLC pattern
    if (len >= 5) {
        uint8_t dlc = data[4];
        if (dlc <= 8 && len >= 5 + dlc) {
            return true;
        }
    }
    
    return false;
}

bool ProtocolAnalyzer::looksLikeIsoTp(const uint8_t* data, size_t len) {
    if (len < 1) return false;
    
    uint8_t pci_type = data[0] >> 4;
    return pci_type <= 3;
}

const std::deque<J1939Frame>& ProtocolAnalyzer::getJ1939Frames() const {
    return j1939_frames_;
}

const std::vector<ProtocolHypothesis>& ProtocolAnalyzer::getHypotheses() const {
    return hypotheses_;
}

ProtocolAnalyzer::Stats ProtocolAnalyzer::getStats() const {
    return stats_;
}

void ProtocolAnalyzer::clear() {
    j1939_frames_.clear();
    hypotheses_.clear();
    stats_ = {};
    iso_tp_state_ = {};
}

void ProtocolAnalyzer::generateHypotheses() {
    hypotheses_.clear();
    
    if (stats_.j1939_frames_decoded > 10) {
        hypotheses_.push_back({
            ProtocolHypothesis::J1939,
            "J1939 frames detected (" + std::to_string(stats_.j1939_frames_decoded) + " frames)",
            0.8,
            {},
            "HIGH CONFIDENCE: Multiple valid J1939 frames decoded from USB payload"
        });
    }
    
    if (stats_.iso_tp_frames_decoded > 0) {
        hypotheses_.push_back({
            ProtocolHypothesis::ISO15765,
            "ISO-TP (ISO 15765-2) frames detected",
            0.6,
            {},
            "ISO-TP reassembly successful - possible UDS/KWP2000 on top"
        });
    }
    
    // Analyze frame patterns
    if (!j1939_frames_.empty()) {
        // Check for common PGNs
        std::vector<uint32_t> pgns;
        for (const auto& f : j1939_frames_) {
            pgns.push_back(f.getPGN());
        }
        
        // Look for known PGNs
        static const std::pair<uint32_t, const char*> known_pgns[] = {
            {0xFEE0, "Engine Temperature"}, {0xFEEE, "Engine Fluid Level"},
            {0xFEFC, "Vehicle Speed"}, {0xFEF1, "Engine Speed"},
            {0xF004, "EEC1"}, {0xF003, "EEC2"}, {0xF001, "EBC1"},
            {0xF000, "EBC2"}, {0xFECA, "Tachograph"}, {0xFE6F, "Dash Display"}
        };
        
        for (auto [pgn, name] : known_pgns) {
            if (std::find(pgns.begin(), pgns.end(), pgn) != pgns.end()) {
                hypotheses_.push_back({
                    ProtocolHypothesis::J1939,
                    "Known J1939 PGN detected: 0x" + std::to_string(pgn) + " (" + name + ")",
                    0.9,
                    {},
                    "CONFIRMED: This is a J1939-based vehicle protocol"
                });
            }
        }
    }
}

} // namespace ca3