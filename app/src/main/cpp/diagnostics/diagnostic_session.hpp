#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <optional>
#include "../cat/cat.hpp"
#include "../j1939/j1939.hpp"
#include "../plus1/plus1.hpp"

namespace ca3 {

enum class DiagnosticState {
    IDLE,
    CONNECTING,
    SECURITY_ACCESS,
    ECU_DISCOVERY,
    READY,
    READING_DTCS,
    READING_LIVE_DATA,
    RUNNING_TEST,
    PROGRAMMING,
    ERROR,
    DISCONNECTING
};

enum class SecurityLevel {
    NONE = 0,
    LEVEL_1 = 1,
    LEVEL_2 = 2,
    LEVEL_3 = 3,
    LEVEL_4 = 4
};

struct DTCInfo {
    uint32_t code = 0;
    std::string description;
    std::string status;  // "ACTIVE", "STORED", "PENDING"
    uint8_t severity = 0;  // 0=info, 1=warning, 2=error, 3=critical
    uint64_t timestampNs = 0;
    uint8_t occurrenceCount = 0;
    std::string spn;
    std::string fmi;
};

struct LiveDataParameter {
    uint16_t pid = 0;
    std::string name;
    std::string value;
    std::string units;
    uint64_t timestampNs = 0;
    bool valid = false;
};

struct EcuInfo {
    uint8_t ecuId = 0;
    std::string name;
    std::string partNumber;
    std::string softwareVersion;
    std::string hardwareVersion;
    std::string vin;
    uint8_t protocolVersion = 0;
    bool isResponding = false;
};

struct DiagnosticSessionConfig {
    bool autoConnect = true;
    bool autoSecurityAccess = true;
    SecurityLevel requiredSecurityLevel = SecurityLevel::LEVEL_1;
    uint32_t connectionTimeoutMs = 10000;
    uint32_t commandTimeoutMs = 5000;
    bool enableTesterPresent = true;
    uint32_t testerPresentIntervalMs = 3000;
};

class DiagnosticSession {
public:
    using StateCallback = std::function<void(DiagnosticState)>;
    using ProgressCallback = std::function<void(const std::string&, float)>;
    using DTCCallback = std::function<void(const std::vector<DTCInfo>&)>;
    using LiveDataCallback = std::function<void(const std::vector<LiveDataParameter>&)>;
    using ECUCallback = std::function<void(const std::vector<EcuInfo>&)>;
    using ErrorCallback = std::function<void(const std::string&)>;

    DiagnosticSession() = default;
    virtual ~DiagnosticSession() = default;

    void setStateCallback(StateCallback callback) { stateCallback_ = std::move(callback); }
    void setProgressCallback(ProgressCallback callback) { progressCallback_ = std::move(callback); }
    void setDTCCallback(DTCCallback callback) { dtcCallback_ = std::move(callback); }
    void setLiveDataCallback(LiveDataCallback callback) { liveDataCallback_ = std::move(callback); }
    void setECUCallback(ECUCallback callback) { ecuCallback_ = std::move(callback); }
    void setErrorCallback(ErrorCallback callback) { errorCallback_ = std::move(callback); }

    virtual bool start(const DiagnosticSessionConfig& config) = 0;
    virtual void stop() = 0;

    virtual bool connect() = 0;
    virtual void disconnect() = 0;

    virtual bool requestSecurityAccess(SecurityLevel level) = 0;
    virtual bool sendSecurityKey(const std::vector<uint8_t>& key) = 0;

    virtual std::vector<DTCInfo> readDTCs() = 0;
    virtual std::vector<DTCInfo> readPendingDTCs() = 0;
    virtual std::vector<DTCInfo> readStoredDTCs() = 0;
    virtual bool clearDTCs() = 0;

    virtual std::vector<LiveDataParameter> readLiveData(const std::vector<uint16_t>& pids) = 0;
    virtual bool startLiveDataStream(const std::vector<uint16_t>& pids, uint32_t intervalMs) = 0;
    virtual void stopLiveDataStream() = 0;

    virtual std::vector<EcuInfo> discoverECUs() = 0;
    virtual bool identifyECU(uint8_t ecuId, EcuInfo& info) = 0;

    virtual bool readDataByIdentifier(uint16_t pid, std::vector<uint8_t>& data) = 0;
    virtual bool writeDataByIdentifier(uint16_t pid, const std::vector<uint8_t>& data) = 0;

    virtual bool startRoutine(uint16_t routineId, const std::vector<uint8_t>& params) = 0;
    virtual bool stopRoutine(uint16_t routineId) = 0;
    virtual bool getRoutineResult(uint16_t routineId, std::vector<uint8_t>& result) = 0;

    virtual bool requestDownload(uint32_t address, uint32_t size) = 0;
    virtual bool transferData(uint8_t sequence, const std::vector<uint8_t>& data) = 0;
    virtual bool requestTransferExit() = 0;

    DiagnosticState getState() const { return state_; }
    const std::string& getLastError() const { return lastError_; }
    bool isConnected() const { return state_ == DiagnosticState::READY; }

protected:
    void setState(DiagnosticState newState) {
        if (state_ != newState) {
            state_ = newState;
            if (stateCallback_) stateCallback_(state_);
        }
    }

    void setError(const std::string& error) {
        lastError_ = error;
        if (errorCallback_) errorCallback_(error);
    }

    void setProgress(const std::string& message, float progress) {
        if (progressCallback_) progressCallback_(message, progress);
    }

    DiagnosticState state_ = DiagnosticState::IDLE;
    std::string lastError_;

    StateCallback stateCallback_;
    ProgressCallback progressCallback_;
    DTCCallback dtcCallback_;
    LiveDataCallback liveDataCallback_;
    ECUCallback ecuCallback_;
    ErrorCallback errorCallback_;
};

} // namespace ca3