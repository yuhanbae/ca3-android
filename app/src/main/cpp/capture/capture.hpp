#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <fstream>
#include <memory>
#include <mutex>
#include <atomic>

namespace ca3 {

enum class CaptureDirection {
    TX,  // Host to device
    RX   // Device to host
};

enum class CaptureTransferType {
    CONTROL = 0,
    BULK = 1,
    INTERRUPT = 2,
    ISOCHRONOUS = 3
};

struct CaptureRecord {
    uint64_t timestampNs = 0;
    CaptureDirection direction = CaptureDirection::RX;
    CaptureTransferType transferType = CaptureTransferType::BULK;
    uint8_t endpoint = 0;
    uint8_t requestType = 0;
    uint8_t request = 0;
    uint16_t value = 0;
    uint16_t index = 0;
    std::vector<uint8_t> payload;
};

class CaptureWriter {
public:
    virtual ~CaptureWriter() = default;
    virtual bool open(const std::string& path) = 0;
    virtual void close() = 0;
    virtual bool writeRecord(const CaptureRecord& record) = 0;
    virtual bool isOpen() const = 0;
};

class BinaryCaptureWriter : public CaptureWriter {
public:
    BinaryCaptureWriter() = default;
    ~BinaryCaptureWriter() override { close(); }

    bool open(const std::string& path) override;
    void close() override;
    bool writeRecord(const CaptureRecord& record) override;
    bool isOpen() const override { return file_.is_open(); }

private:
    std::ofstream file_;
    std::mutex mutex_;
    uint64_t recordCount_ = 0;

    static constexpr char MAGIC[] = "C3CAPv1\n";
};

class JsonlCaptureWriter : public CaptureWriter {
public:
    JsonlCaptureWriter() = default;
    ~JsonlCaptureWriter() override { close(); }

    bool open(const std::string& path) override;
    void close() override;
    bool writeRecord(const CaptureRecord& record) override;
    bool isOpen() const override { return file_.is_open(); }

private:
    std::ofstream file_;
    std::mutex mutex_;
    uint64_t recordCount_ = 0;
};

class CsvCaptureWriter : public CaptureWriter {
public:
    CsvCaptureWriter() = default;
    ~CsvCaptureWriter() override { close(); }

    bool open(const std::string& path) override;
    void close() override;
    bool writeRecord(const CaptureRecord& record) override;
    bool isOpen() const override { return file_.is_open(); }

private:
    std::ofstream file_;
    std::mutex mutex_;
    uint64_t recordCount_ = 0;
    bool headerWritten_ = false;
};

class CaptureManager {
public:
    enum class Format {
        BINARY,
        JSONL,
        CSV
    };

    CaptureManager() = default;
    ~CaptureManager() { stopCapture(); }

    bool startCapture(const std::string& directory, Format format = Format::BINARY);
    void stopCapture();
    bool isCapturing() const { return isCapturing_.load(); }
    uint64_t getRecordCount() const { return recordCount_.load(); }

    void record(const CaptureRecord& record);
    void recordUsb(const uint8_t* data, size_t length, uint64_t timestampNs,
                   CaptureDirection direction, CaptureTransferType type,
                   uint8_t endpoint, uint8_t requestType = 0, uint8_t request = 0,
                   uint16_t value = 0, uint16_t index = 0);

    std::string getCurrentCapturePath() const;

private:
    std::atomic<bool> isCapturing_{false};
    std::atomic<uint64_t> recordCount_{0};
    std::unique_ptr<CaptureWriter> writer_;
    std::string currentPath_;
    Format currentFormat_ = Format::BINARY;
    std::mutex mutex_;
};

} // namespace ca3