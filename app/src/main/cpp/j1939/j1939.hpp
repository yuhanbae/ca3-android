#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <chrono>
#include <functional>
#include "ca3_transport.hpp"

namespace ca3 {

struct J1939Frame {
    uint32_t canId = 0;      // 29-bit extended CAN ID
    uint8_t data[8] = {0};
    uint8_t dlc = 0;         // Data Length Code (0-8)
    uint64_t timestampNs = 0;

    uint32_t getPGN() const {
        uint8_t pf = (canId >> 16) & 0xFF;
        uint8_t ps = (canId >> 8) & 0xFF;
        if (pf < 240) {
            return (pf << 8) | ps;
        } else {
            return (pf << 8);
        }
    }

    uint8_t getPriority() const { return (canId >> 26) & 0x07; }
    uint8_t getSourceAddress() const { return canId & 0xFF; }
    uint8_t getDestinationAddress() const {
        uint8_t pf = (canId >> 16) & 0xFF;
        return (pf < 240) ? ((canId >> 8) & 0xFF) : 0xFF;
    }
    bool isExtended() const { return (canId & 0x80000000) != 0; }
    bool isPDU1() const { return ((canId >> 16) & 0xFF) < 240; }
    bool isPDU2() const { return !isPDU1(); }
};

struct SPN {
    uint32_t number = 0;
    std::string name;
    std::string description;
    uint8_t startBit = 0;
    uint8_t bitLength = 0;
    double resolution = 1.0;
    double offset = 0.0;
    std::string units;

    double extract(const uint8_t* data) const {
        uint64_t raw = 0;
        for (int i = 0; i < 8; ++i) {
            raw |= (static_cast<uint64_t>(data[i]) << (i * 8));
        }
        uint64_t mask = (bitLength == 64) ? ~0ULL : ((1ULL << bitLength) - 1);
        uint64_t value = (raw >> startBit) & mask;
        return value * resolution + offset;
    }
};

struct PGN {
    uint32_t number = 0;
    std::string acronym;
    std::string name;
    uint8_t dlc = 8;
    uint16_t transmissionRateMs = 0;
    std::vector<SPN> spns;

    std::vector<std::pair<std::string, double>> decode(const uint8_t* data) const {
        std::vector<std::pair<std::string, double>> results;
        for (const auto& spn : spns) {
            results.emplace_back(spn.name, spn.extract(data));
        }
        return results;
    }
};

class J1939Database {
public:
    static const J1939Database& instance() {
        static J1939Database db;
        return db;
    }

    std::optional<PGN> getPGN(uint32_t pgn) const;
    std::optional<SPN> getSPN(uint32_t spn) const;

private:
    J1939Database();
    std::unordered_map<uint32_t, PGN> pgm_;
    std::unordered_map<uint32_t, SPN> spn_;
    void initKnownPGNs();
    void initKnownSPNs();
};

class J1939TransportProtocol {
public:
    struct Session {
        uint32_t pgn = 0;
        uint8_t source = 0;
        uint8_t dest = 0;
        uint16_t totalBytes = 0;
        uint8_t numPackets = 0;
        uint8_t nextPacket = 0;
        std::vector<uint8_t> buffer;
        uint64_t startTime = 0;
        bool active = false;
    };

    static constexpr uint32_t TP_CM = 0xEC00;
    static constexpr uint32_t TP_DT = 0xEB00;

    enum ControlByte : uint8_t {
        RTS = 16,
        CTS = 17,
        EOM = 19,
        BAM = 32,
        ABORT = 255
    };

    std::optional<std::vector<uint8_t>> processFrame(const J1939Frame& frame);
    const std::unordered_map<uint32_t, Session>& getSessions() const { return sessions_; }

private:
    std::unordered_map<uint32_t, Session> sessions_;

    std::optional<std::vector<uint8_t>> processControlFrame(const J1939Frame& frame);
    std::optional<std::vector<uint8_t>> processDataFrame(const J1939Frame& frame);
};

class J1939Processor {
public:
    using FrameCallback = std::function<void(const J1939Frame&)>;
    using PGNCallback = std::function<void(uint32_t, const std::vector<uint8_t>&)>;

    J1939Processor();
    ~J1939Processor();

    void setFrameCallback(FrameCallback callback);
    void setPGNCallback(PGNCallback callback);

    void feedCANFrame(uint32_t canId, const uint8_t* data, uint8_t dlc, uint64_t timestampNs);
    void feedUSBPayload(const uint8_t* data, size_t len, uint64_t timestampNs);

    const std::vector<J1939Frame>& getFrames() const { return frames_; }
    const J1939TransportProtocol& getTransportProtocol() const { return tp_; }

private:
    FrameCallback frameCallback_;
    PGNCallback pgnCallback_;
    std::vector<J1939Frame> frames_;
    J1939TransportProtocol tp_;

    std::optional<J1939Frame> tryDecodeJ1939(const uint8_t* data, size_t len, uint64_t timestampNs);
    std::vector<uint8_t> tryDecodeIsoTp(const uint8_t* data, size_t len);

    struct IsoTpState {
        std::vector<uint8_t> buffer;
        bool inProgress = false;
        uint32_t expectedLen = 0;
        uint8_t seqNum = 0;
        uint64_t firstTimestamp = 0;
    } isoTpState_;
};

} // namespace ca3