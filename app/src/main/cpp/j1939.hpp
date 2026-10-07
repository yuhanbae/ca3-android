#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>

namespace ca3 {

// J1939 Protocol Implementation
// Based on SAE J1939 standard

struct J1939Frame {
    uint32_t can_id;      // 29-bit extended CAN ID
    uint8_t data[8];
    uint8_t dlc;          // Data Length Code (0-8)
    uint64_t timestamp_ns;
    
    // PGN extraction
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
    
    // Priority (3 bits)
    uint8_t getPriority() const {
        return (can_id >> 26) & 0x07;
    }
    
    // Source Address
    uint8_t getSourceAddress() const {
        return can_id & 0xFF;
    }
    
    // Destination Address (for PDU1)
    uint8_t getDestinationAddress() const {
        uint8_t pf = (can_id >> 16) & 0xFF;
        if (pf < 240) {
            return (can_id >> 8) & 0xFF;
        }
        return 0xFF; // Global for PDU2
    }
    
    bool isExtended() const {
        return (can_id & 0x80000000) != 0;
    }
    
    bool isPDU1() const {
        return ((can_id >> 16) & 0xFF) < 240;
    }
    
    bool isPDU2() const {
        return !isPDU1();
    }
};

// SPN (Suspect Parameter Number) definition
struct SPN {
    uint32_t number;
    std::string name;
    std::string description;
    uint8_t start_bit;
    uint8_t bit_length;
    double resolution;
    double offset;
    std::string units;
    
    // Extract value from 8-byte data
    double extract(const uint8_t* data) const {
        uint64_t raw = 0;
        for (int i = 0; i < 8; ++i) {
            raw |= (uint64_t)data[i] << (i * 8);
        }
        
        uint64_t mask = (bit_length == 64) ? ~0ULL : ((1ULL << bit_length) - 1);
        uint64_t value = (raw >> start_bit) & mask;
        
        return value * resolution + offset;
    }
};

// PGN (Parameter Group Number) definition
struct PGN {
    uint32_t number;
    std::string acronym;
    std::string name;
    uint8_t dlc;
    uint16_t transmission_rate_ms; // 0 = on request
    std::vector<SPN> spns;
    
    // Decode frame data into SPN values
    std::vector<std::pair<std::string, double>> decode(const uint8_t* data) const {
        std::vector<std::pair<std::string, double>> results;
        for (const auto& spn : spns) {
            results.emplace_back(spn.name, spn.extract(data));
        }
        return results;
    }
};

// J1939 Database - common PGNs and SPNs
class J1939Database {
public:
    static const J1939Database& instance() {
        static J1939Database db;
        return db;
    }
    
    std::optional<PGN> getPGN(uint32_t pgn) const {
        auto it = pgm_.find(pgn);
        if (it != pgm_.end()) return it->second;
        return std::nullopt;
    }
    
    std::optional<SPN> getSPN(uint32_t spn) const {
        auto it = spn_.find(spn);
        if (it != spn_.end()) return it->second;
        return std::nullopt;
    }
    
    const std::unordered_map<uint32_t, PGN>& allPGNs() const { return pgm_; }
    const std::unordered_map<uint32_t, SPN>& allSPNs() const { return spn_; }

private:
    J1939Database() {
        initKnownPGNs();
        initKnownSPNs();
    }
    
    void initKnownPGNs() {
        // Vehicle Speed
        pgm_[0xFEF1] = {0xFEF1, "VD", "Vehicle Distance", 8, 100, {
            {917, "Total Vehicle Distance", "Total vehicle distance", 0, 32, 0.125, 0, "km"},
            {245, "Trip Distance", "Trip distance", 32, 24, 0.125, 0, "km"}
        }};
        
        // Engine Speed
        pgm_[0xF004] = {0xF004, "EEC1", "Electronic Engine Controller 1", 8, 50, {
            {190, "Engine Speed", "Engine speed", 0, 16, 0.125, 0, "rpm"},
            {91, "Accelerator Pedal Position", "Accelerator pedal position", 16, 8, 0.4, 0, "%"},
            {92, "Engine Percent Load", "Engine percent load", 24, 8, 1, 0, "%"}
        }};
        
        // Engine Temperature
        pgm_[0xFEEE] = {0xFEEE, "ET", "Engine Temperature", 8, 1000, {
            {110, "Engine Coolant Temperature", "Engine coolant temperature", 0, 8, 1, -40, "°C"},
            {175, "Engine Oil Temperature", "Engine oil temperature", 8, 8, 1, -40, "°C"},
            {174, "Fuel Temperature", "Fuel temperature", 16, 8, 1, -40, "°C"}
        }};
        
        // Dash Display
        pgm_[0xFE6F] = {0xFE6F, "DD", "Dash Display", 8, 1000, {
            {2979, "Vehicle Speed", "Vehicle speed", 0, 16, 0.00390625, 0, "km/h"},
            {84, "Wheel-Based Vehicle Speed", "Wheel-based vehicle speed", 16, 16, 0.00390625, 0, "km/h"}
        }};
        
        // Tachograph
        pgm_[0xFECA] = {0xFECA, "TG", "Tachograph", 8, 1000, {
            {247, "Total Engine Hours", "Total engine hours", 0, 24, 0.05, 0, "h"},
            {1641, "Trip Engine Hours", "Trip engine hours", 24, 24, 0.05, 0, "h"}
        }};
        
        // Brakes
        pgm_[0xF001] = {0xF001, "EBC1", "Electronic Brake Controller 1", 8, 50, {
            {561, "ASR Engine Control", "ASR engine control", 0, 2, 1, 0, ""},
            {562, "ASR Brake Control", "ASR brake control", 2, 2, 1, 0, ""}
        }};
    }
    
    void initKnownSPNs() {
        // Common SPNs
        spn_[190] = {190, "Engine Speed", "Engine speed", 0, 16, 0.125, 0, "rpm"};
        spn_[91] = {91, "Accelerator Pedal Position", "Accelerator pedal position", 0, 8, 0.4, 0, "%"};
        spn_[92] = {92, "Engine Percent Load", "Engine percent load", 0, 8, 1, 0, "%"};
        spn_[110] = {110, "Engine Coolant Temperature", "Engine coolant temperature", 0, 8, 1, -40, "°C"};
        spn_[175] = {175, "Engine Oil Temperature", "Engine oil temperature", 0, 8, 1, -40, "°C"};
        spn_[174] = {174, "Fuel Temperature", "Fuel temperature", 0, 8, 1, -40, "°C"};
        spn_[917] = {917, "Total Vehicle Distance", "Total vehicle distance", 0, 32, 0.125, 0, "km"};
        spn_[245] = {245, "Trip Distance", "Trip distance", 0, 24, 0.125, 0, "km"};
        spn_[2979] = {2979, "Vehicle Speed", "Vehicle speed", 0, 16, 0.00390625, 0, "km/h"};
        spn_[84] = {84, "Wheel-Based Vehicle Speed", "Wheel-based vehicle speed", 0, 16, 0.00390625, 0, "km/h"};
        spn_[247] = {247, "Total Engine Hours", "Total engine hours", 0, 24, 0.05, 0, "h"};
        spn_[561] = {561, "ASR Engine Control", "ASR engine control", 0, 2, 1, 0, ""};
        spn_[562] = {562, "ASR Brake Control", "ASR brake control", 0, 2, 1, 0, ""};
    }
    
    std::unordered_map<uint32_t, PGN> pgm_;
    std::unordered_map<uint32_t, SPN> spn_;
};

// J1939 Transport Protocol (BAM - Broadcast Announce Message)
class J1939TransportProtocol {
public:
    struct Session {
        uint32_t pgn;
        uint8_t source;
        uint8_t dest;
        uint16_t total_bytes;
        uint8_t num_packets;
        uint8_t next_packet;
        std::vector<uint8_t> buffer;
        uint64_t start_time;
        bool active = false;
    };
    
    static constexpr uint32_t TP_CM = 0xEC00;  // Connection Management
    static constexpr uint32_t TP_DT = 0xEB00;  // Data Transfer
    
    enum ControlByte {
        RTS = 16,   // Request To Send
        CTS = 17,   // Clear To Send
        EOM = 19,   // End Of Message
        BAM = 32,   // Broadcast Announce Message
        ABORT = 255
    };
    
    // Process incoming J1939 frame for TP
    std::optional<std::vector<uint8_t>> processFrame(const J1939Frame& frame) {
        uint32_t pgn = frame.getPGN();
        
        if (pgn == TP_CM) {
            return processControlFrame(frame);
        } else if (pgn == TP_DT) {
            return processDataFrame(frame);
        }
        return std::nullopt;
    }
    
    // Get active sessions
    const std::unordered_map<uint32_t, Session>& getSessions() const {
        return sessions_;
    }

private:
    std::unordered_map<uint32_t, Session> sessions_;
    
    std::optional<std::vector<uint8_t>> processControlFrame(const J1939Frame& frame) {
        if (frame.dlc < 8) return std::nullopt;
        
        uint8_t control = frame.data[0];
        uint16_t total_bytes = frame.data[1] | (frame.data[2] << 8);
        uint8_t num_packets = frame.data[3];
        uint32_t msg_pgn = frame.data[4] | (frame.data[5] << 8) | (frame.data[6] << 16);
        
        uint32_t session_key = frame.getSourceAddress();
        
        switch (control) {
            case RTS:
            case BAM: {
                Session& session = sessions_[session_key];
                session.pgn = msg_pgn;
                session.source = frame.getSourceAddress();
                session.dest = frame.getDestinationAddress();
                session.total_bytes = total_bytes;
                session.num_packets = num_packets;
                session.next_packet = 1;
                session.buffer.resize(total_bytes);
                session.start_time = frame.timestamp_ns;
                session.active = true;
                break;
            }
            case CTS: {
                auto it = sessions_.find(session_key);
                if (it != sessions_.end()) {
                    it->second.next_packet = frame.data[1];
                }
                break;
            }
            case EOM: {
                auto it = sessions_.find(session_key);
                if (it != sessions_.end() && it->second.buffer.size() == it->second.total_bytes) {
                    std::vector<uint8_t> result = it->second.buffer;
                    sessions_.erase(it);
                    return result;
                }
                sessions_.erase(it);
                break;
            }
            case ABORT: {
                sessions_.erase(session_key);
                break;
            }
        }
        return std::nullopt;
    }
    
    std::optional<std::vector<uint8_t>> processDataFrame(const J1939Frame& frame) {
        if (frame.dlc < 1) return std::nullopt;
        
        uint8_t seq = frame.data[0];
        uint32_t session_key = frame.getSourceAddress();
        
        auto it = sessions_.find(session_key);
        if (it == sessions_.end() || !it->second.active) return std::nullopt;
        
        Session& session = it->second;
        if (seq != session.next_packet) {
            // Sequence error - abort
            sessions_.erase(it);
            return std::nullopt;
        }
        
        // Copy data (7 bytes per frame)
        size_t offset = (seq - 1) * 7;
        size_t copy_len = std::min<size_t>(7, frame.dlc - 1);
        if (offset + copy_len <= session.buffer.size()) {
            std::copy(frame.data + 1, frame.data + 1 + copy_len, session.buffer.begin() + offset);
        }
        
        session.next_packet++;
        
        // Check for EOM (handled in CM)
        return std::nullopt;
    }
};

} // namespace ca3