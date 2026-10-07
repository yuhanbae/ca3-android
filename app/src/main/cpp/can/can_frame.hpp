#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <optional>

namespace ca3 {

struct CANFrame {
    uint32_t canId = 0;       // 11-bit or 29-bit CAN ID
    uint8_t data[8] = {0};
    uint8_t dlc = 0;          // Data Length Code (0-8)
    uint64_t timestampNs = 0;
    bool extended = false;    // 29-bit extended frame
    bool rtr = false;         // Remote Transmission Request
    bool fd = false;          // CAN FD frame
    uint8_t fdf = 0;          // FD Format
    uint8_t brs = 0;          // Bit Rate Switch
    uint8_t esi = 0;          // Error State Indicator

    bool isExtended() const { return extended; }
    bool isStandard() const { return !extended; }
    bool isRTR() const { return rtr; }
    bool isFD() const { return fd; }

    uint32_t getStandardId() const {
        return canId & 0x7FF;
    }

    uint32_t getExtendedId() const {
        return canId & 0x1FFFFFFF;
    }
};

class CANBus {
public:
    using FrameCallback = std::function<void(const CANFrame&)>;
    using ErrorCallback = std::function<void(const std::string&)>;

    virtual ~CANBus() = default;

    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool send(const CANFrame& frame) = 0;
    virtual void setFrameCallback(FrameCallback callback) = 0;
    virtual void setErrorCallback(ErrorCallback callback) = 0;
    virtual bool isOpen() const = 0;
};

class CA3CANBus : public CANBus {
public:
    using FrameCallback = std::function<void(const CANFrame&)>;
    using ErrorCallback = std::function<void(const std::string&)>;

    CA3CANBus() = default;
    ~CA3CANBus() override { close(); }

    bool open() override;
    void close() override;
    bool send(const CANFrame& frame) override;
    void setFrameCallback(FrameCallback callback) override;
    void setErrorCallback(ErrorCallback callback) override;
    bool isOpen() const override;

private:
    FrameCallback frameCallback_;
    ErrorCallback errorCallback_;
    bool isOpen_ = false;
};

struct CANBitTiming {
    uint32_t bitrate = 250000;
    uint8_t sjw = 1;
    uint8_t tseg1 = 14;
    uint8_t tseg2 = 6;
    uint8_t propSeg = 0;
    uint8_t phaseSeg1 = 0;
    uint8_t phaseSeg2 = 0;
    bool tripleSampling = false;

    static CANBitTiming ciA_250k() {
        return {250000, 1, 14, 6};
    }

    static CANBitTiming ciA_500k() {
        return {500000, 1, 12, 4};
    }

    static CANBitTiming j1939_250k() {
        return {250000, 1, 14, 6};
    }
};

} // namespace ca3