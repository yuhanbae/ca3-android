#include "j1939.hpp"
#include <algorithm>
#include <cstring>
#include <android/log.h>

#define LOG_TAG "J1939"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace ca3 {

// J1939 Database Implementation
J1939Database::J1939Database() {
    initKnownPGNs();
    initKnownSPNs();
}

void J1939Database::initKnownPGNs() {
    // Vehicle Distance
    pgm_[0xFEF1] = {0xFEF1, "VD", "Vehicle Distance", 8, 100, {
        {917, "Total Vehicle Distance", "Total vehicle distance", 0, 32, 0.125, 0, "km"},
        {245, "Trip Distance", "Trip distance", 32, 24, 0.125, 0, "km"}
    }};

    // Engine Speed (EEC1)
    pgm_[0xF004] = {0xF004, "EEC1", "Electronic Engine Controller 1", 8, 50, {
        {190, "Engine Speed", "Engine speed", 0, 16, 0.125, 0, "rpm"},
        {91, "Accelerator Pedal Position 1", "Accelerator pedal position 1", 16, 8, 0.4, 0, "%"},
        {92, "Engine Percent Load At Current Speed", "Engine percent load", 24, 8, 1, 0, "%"}
    }};

    // Engine Temperature
    pgm_[0xFEEE] = {0xFEEE, "ET", "Engine Temperature", 8, 1000, {
        {110, "Engine Coolant Temperature", "Engine coolant temperature", 0, 8, 1, -40, "°C"},
        {175, "Engine Oil Temperature 1", "Engine oil temperature 1", 8, 8, 1, -40, "°C"},
        {174, "Engine Fuel Temperature 1", "Engine fuel temperature 1", 16, 8, 1, -40, "°C"}
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

    // Brakes (EBC1)
    pgm_[0xF001] = {0xF001, "EBC1", "Electronic Brake Controller 1", 8, 50, {
        {561, "ASR Engine Control", "ASR engine control", 0, 2, 1, 0, ""},
        {562, "ASR Brake Control", "ASR brake control", 2, 2, 1, 0, ""}
    }};
}

void J1939Database::initKnownSPNs() {
    spn_[190] = {190, "Engine Speed", "Engine speed", 0, 16, 0.125, 0, "rpm"};
    spn_[91] = {91, "Accelerator Pedal Position 1", "Accelerator pedal position 1", 0, 8, 0.4, 0, "%"};
    spn_[92] = {92, "Engine Percent Load At Current Speed", "Engine percent load", 0, 8, 1, 0, "%"};
    spn_[110] = {110, "Engine Coolant Temperature", "Engine coolant temperature", 0, 8, 1, -40, "°C"};
    spn_[175] = {175, "Engine Oil Temperature 1", "Engine oil temperature 1", 0, 8, 1, -40, "°C"};
    spn_[174] = {174, "Engine Fuel Temperature 1", "Engine fuel temperature 1", 0, 8, 1, -40, "°C"};
    spn_[917] = {917, "Total Vehicle Distance", "Total vehicle distance", 0, 32, 0.125, 0, "km"};
    spn_[245] = {245, "Trip Distance", "Trip distance", 0, 24, 0.125, 0, "km"};
    spn_[2979] = {2979, "Vehicle Speed", "Vehicle speed", 0, 16, 0.00390625, 0, "km/h"};
    spn_[84] = {84, "Wheel-Based Vehicle Speed", "Wheel-based vehicle speed", 0, 16, 0.00390625, 0, "km/h"};
    spn_[247] = {247, "Total Engine Hours", "Total engine hours", 0, 24, 0.05, 0, "h"};
    spn_[561] = {561, "ASR Engine Control", "ASR engine control", 0, 2, 1, 0, ""};
    spn_[562] = {562, "ASR Brake Control", "ASR brake control", 0, 2, 1, 0, ""};
}

std::optional<PGN> J1939Database::getPGN(uint32_t pgn) const {
    auto it = pgm_.find(pgn);
    if (it != pgm_.end()) return it->second;
    return std::nullopt;
}

std::optional<SPN> J1939Database::getSPN(uint32_t spn) const {
    auto it = spn_.find(spn);
    if (it != spn_.end()) return it->second;
    return std::nullopt;
}

// J1939 Transport Protocol
std::optional<std::vector<uint8_t>> J1939TransportProtocol::processFrame(const J1939Frame& frame) {
    uint32_t pgn = frame.getPGN();

    if (pgn == TP_CM) {
        return processControlFrame(frame);
    } else if (pgn == TP_DT) {
        return processDataFrame(frame);
    }
    return std::nullopt;
}

std::optional<std::vector<uint8_t>> J1939TransportProtocol::processControlFrame(const J1939Frame& frame) {
    if (frame.dlc < 8) return std::nullopt;

    uint8_t control = frame.data[0];
    uint16_t totalBytes = frame.data[1] | (frame.data[2] << 8);
    uint8_t numPackets = frame.data[3];
    uint32_t msgPgn = frame.data[4] | (frame.data[5] << 8) | (frame.data[6] << 16);

    uint32_t sessionKey = frame.getSourceAddress();

    switch (control) {
        case RTS:
        case BAM: {
            Session& session = sessions_[sessionKey];
            session.pgn = msgPgn;
            session.source = frame.getSourceAddress();
            session.dest = frame.getDestinationAddress();
            session.totalBytes = totalBytes;
            session.numPackets = numPackets;
            session.nextPacket = 1;
            session.buffer.resize(totalBytes);
            session.startTime = frame.timestampNs;
            session.active = true;
            break;
        }
        case CTS: {
            auto it = sessions_.find(sessionKey);
            if (it != sessions_.end()) {
                it->second.nextPacket = frame.data[1];
            }
            break;
        }
        case EOM: {
            auto it = sessions_.find(sessionKey);
            if (it != sessions_.end() && it->second.buffer.size() == it->second.totalBytes) {
                std::vector<uint8_t> result = it->second.buffer;
                sessions_.erase(it);
                return result;
            }
            sessions_.erase(it);
            break;
        }
        case ABORT: {
            sessions_.erase(sessionKey);
            break;
        }
    }
    return std::nullopt;
}

std::optional<std::vector<uint8_t>> J1939TransportProtocol::processDataFrame(const J1939Frame& frame) {
    if (frame.dlc < 1) return std::nullopt;

    uint8_t seq = frame.data[0];
    uint32_t sessionKey = frame.getSourceAddress();

    auto it = sessions_.find(sessionKey);
    if (it == sessions_.end() || !it->second.active) return std::nullopt;

    Session& session = it->second;
    if (seq != session.nextPacket) {
        sessions_.erase(it);
        return std::nullopt;
    }

    size_t offset = (seq - 1) * 7;
    size_t copyLen = std::min<size_t>(7, frame.dlc - 1);
    if (offset + copyLen <= session.buffer.size()) {
        std::copy(frame.data + 1, frame.data + 1 + copyLen, session.buffer.begin() + offset);
    }

    session.nextPacket++;

    return std::nullopt;
}

// J1939 Processor
J1939Processor::J1939Processor() {}

J1939Processor::~J1939Processor() {}

void J1939Processor::setFrameCallback(FrameCallback callback) {
    frameCallback_ = std::move(callback);
}

void J1939Processor::setPGNCallback(PGNCallback callback) {
    pgnCallback_ = std::move(callback);
}

void J1939Processor::feedCANFrame(uint32_t canId, const uint8_t* data, uint8_t dlc, uint64_t timestampNs) {
    J1939Frame frame;
    frame.canId = canId | 0x80000000; // Extended frame
    frame.dlc = std::min<uint8_t>(dlc, 8);
    frame.timestampNs = timestampNs;
    std::memcpy(frame.data, data, frame.dlc);

    if (frameCallback_) frameCallback_(frame);

    // Try transport protocol
    auto tpResult = tp_.processFrame(frame);
    if (tpResult) {
        if (pgnCallback_) {
            pgnCallback_(frame.getPGN(), *tpResult);
        }
    }

    frames_.push_back(frame);
    if (frames_.size() > 10000) frames_.erase(frames_.begin());
}

void J1939Processor::feedUSBPayload(const uint8_t* data, size_t len, uint64_t timestampNs) {
    // Try to decode as J1939 frames
    if (len == 13) {
        // Hypothesis: 4-byte CAN ID + DLC + 8 bytes data
        uint32_t canId = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
        uint8_t dlc = data[4];
        if (dlc <= 8 && len >= 5 + dlc) {
            J1939Frame frame;
            frame.canId = canId | 0x80000000;
            frame.dlc = dlc;
            frame.timestampNs = timestampNs;
            std::memcpy(frame.data, data + 5, dlc);
            feedCANFrame(canId, frame.data, dlc, timestampNs);
        }
    } else if (len >= 11) {
        // Hypothesis: 3-byte PGN + SA + DLC + data
        uint32_t pgn = data[0] | (data[1] << 8) | (data[2] << 16);
        uint8_t sa = data[3];
        uint8_t dlc = data[4];
        if (dlc <= 8 && len >= 5 + dlc) {
            J1939Frame frame;
            frame.canId = (pgn << 8) | sa | 0x80000000;
            frame.dlc = dlc;
            frame.timestampNs = timestampNs;
            std::memcpy(frame.data, data + 5, dlc);
            feedCANFrame((pgn << 8) | sa, frame.data, dlc, timestampNs);
        }
    }
}

std::optional<J1939Frame> J1939Processor::tryDecodeJ1939(const uint8_t* data, size_t len, uint64_t timestampNs) {
    // Multiple hypotheses for CA3 J1939 encapsulation
    if (len == 13) {
        uint32_t canId = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
        uint8_t dlc = data[4];
        if (dlc <= 8) {
            J1939Frame frame;
            frame.canId = canId | 0x80000000;
            frame.dlc = dlc;
            frame.timestampNs = timestampNs;
            std::memcpy(frame.data, data + 5, dlc);
            return frame;
        }
    }
    return std::nullopt;
}

std::vector<uint8_t> J1939Processor::tryDecodeIsoTp(const uint8_t* data, size_t len) {
    if (len < 1) return {};

    uint8_t pciType = data[0] >> 4;

    switch (pciType) {
        case 0: { // Single Frame
            if (len >= 2) {
                uint8_t dlc = data[0] & 0x0F;
                if (dlc <= 7 && len >= 1 + dlc) {
                    std::vector<uint8_t> result(data + 1, data + 1 + dlc);
                    isoTpState_ = {};
                    return result;
                }
            }
            break;
        }
        case 1: { // First Frame
            if (len >= 3) {
                uint16_t totalLen = ((data[0] & 0x0F) << 8) | data[1];
                isoTpState_.buffer.clear();
                isoTpState_.buffer.reserve(totalLen);
                isoTpState_.inProgress = true;
                isoTpState_.expectedLen = totalLen;
                isoTpState_.seqNum = 0;
                isoTpState_.firstTimestamp = 0;

                size_t firstData = std::min<size_t>(len - 2, 6);
                isoTpState_.buffer.insert(isoTpState_.buffer.end(), data + 2, data + 2 + firstData);
            }
            break;
        }
        case 2: { // Consecutive Frame
            if (isoTpState_.inProgress && len >= 2) {
                uint8_t seq = data[0] & 0x0F;
                if (seq == ((isoTpState_.seqNum + 1) & 0x0F)) {
                    isoTpState_.buffer.insert(isoTpState_.buffer.end(), data + 1, data + len);
                    isoTpState_.seqNum = seq;

                    if (isoTpState_.buffer.size() >= isoTpState_.expectedLen) {
                        std::vector<uint8_t> result = isoTpState_.buffer;
                        result.resize(isoTpState_.expectedLen);
                        isoTpState_ = {};
                        return result;
                    }
                } else {
                    isoTpState_ = {};
                }
            }
            break;
        }
        case 3: { // Flow Control
            break;
        }
    }
    return {};
}

J1939Processor::J1939Processor() {}
J1939Processor::~J1939Processor() {}

void J1939Processor::setFrameCallback(FrameCallback callback) {
    frameCallback_ = std::move(callback);
}

void J1939Processor::setPGNCallback(PGNCallback callback) {
    pgnCallback_ = std::move(callback);
}

} // namespace ca3