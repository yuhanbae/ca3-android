#include "capture.hpp"
#include <android/log.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

#define LOG_TAG "Capture"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace ca3 {

// BinaryCaptureWriter
bool BinaryCaptureWriter::open(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_.open(path, std::ios::binary | std::ios::trunc);
    if (!file_.is_open()) {
        LOGE("Failed to open capture file: %s", path.c_str());
        return false;
    }
    file_.write(MAGIC, sizeof(MAGIC) - 1);
    recordCount_ = 0;
    LOGI("Binary capture opened: %s", path.c_str());
    return true;
}

void BinaryCaptureWriter::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.close();
        LOGI("Binary capture closed, records: %llu", recordCount_);
    }
}

bool BinaryCaptureWriter::writeRecord(const CaptureRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!file_.is_open()) return false;

    try {
        // Write header (32 bytes)
        // timestamp (8 bytes, big endian)
        for (int i = 7; i >= 0; --i) {
            file_.put(static_cast<char>((record.timestampNs >> (i * 8)) & 0xFF));
        }
        // direction (1 byte)
        file_.put(static_cast<char>(record.direction == CaptureDirection::TX ? 0 : 1));
        // transfer type (1 byte)
        file_.put(static_cast<char>(record.transferType));
        // endpoint (1 byte)
        file_.put(static_cast<char>(record.endpoint));
        // request type (1 byte)
        file_.put(static_cast<char>(record.requestType));
        // request (1 byte)
        file_.put(static_cast<char>(record.request));
        // value (2 bytes, big endian)
        file_.put(static_cast<char>((record.value >> 8) & 0xFF));
        file_.put(static_cast<char>(record.value & 0xFF));
        // index (2 bytes, big endian)
        file_.put(static_cast<char>((record.index >> 8) & 0xFF));
        file_.put(static_cast<char>(record.index & 0xFF));
        // payload length (4 bytes, big endian)
        uint32_t len = static_cast<uint32_t>(record.payload.size());
        for (int i = 3; i >= 0; --i) {
            file_.put(static_cast<char>((len >> (i * 8)) & 0xFF));
        }
        // payload
        file_.write(reinterpret_cast<const char*>(record.payload.data()), record.payload.size());

        ++recordCount_;
        return true;
    } catch (const std::exception& e) {
        LOGE("Write record failed: %s", e.what());
        return false;
    }
}

// JsonlCaptureWriter
bool JsonlCaptureWriter::open(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_.open(path, std::ios::trunc);
    if (!file_.is_open()) {
        LOGE("Failed to open JSONL capture file: %s", path.c_str());
        return false;
    }
    recordCount_ = 0;
    LOGI("JSONL capture opened: %s", path.c_str());
    return true;
}

void JsonlCaptureWriter::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.close();
        LOGI("JSONL capture closed, records: %llu", recordCount_);
    }
}

bool JsonlCaptureWriter::writeRecord(const CaptureRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!file_.is_open()) return false;

    try {
        file_ << "{"
              << "\"timestampNs\":" << record.timestampNs << ","
              << "\"direction\":\"" << (record.direction == CaptureDirection::TX ? "TX" : "RX") << "\","
              << "\"transferType\":" << static_cast<int>(record.transferType) << ","
              << "\"endpoint\":\"0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(record.endpoint) << std::dec << "\","
              << "\"requestType\":" << static_cast<int>(record.requestType) << ","
              << "\"request\":" << static_cast<int>(record.request) << ","
              << "\"value\":" << static_cast<int>(record.value) << ","
              << "\"index\":" << static_cast<int>(record.index) << ","
              << "\"length\":" << record.payload.size() << ","
              << "\"dataHex\":\"";

        for (uint8_t byte : record.payload) {
            file_ << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << std::dec << std::setfill(' ');
        }

        file_ << "\"}" << std::endl;

        ++recordCount_;
        return true;
    } catch (const std::exception& e) {
        LOGE("Write JSONL record failed: %s", e.what());
        return false;
    }
}

// CsvCaptureWriter
bool CsvCaptureWriter::open(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_.open(path, std::ios::trunc);
    if (!file_.is_open()) {
        LOGE("Failed to open CSV capture file: %s", path.c_str());
        return false;
    }
    // Write header
    file_ << "timestampNs,direction,transferType,endpoint,requestType,request,value,index,length,dataHex\n";
    headerWritten_ = true;
    recordCount_ = 0;
    LOGI("CSV capture opened: %s", path.c_str());
    return true;
}

void CsvCaptureWriter::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.close();
        LOGI("CSV capture closed, records: %llu", recordCount_);
    }
}

bool CsvCaptureWriter::writeRecord(const CaptureRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!file_.is_open()) return false;

    try {
        file_ << record.timestampNs << ","
              << (record.direction == CaptureDirection::TX ? "TX" : "RX") << ","
              << static_cast<int>(record.transferType) << ","
              << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(record.endpoint) << std::dec << std::setfill(' ') << ","
              << static_cast<int>(record.requestType) << ","
              << static_cast<int>(record.request) << ","
              << static_cast<int>(record.value) << ","
              << static_cast<int>(record.index) << ","
              << record.payload.size() << ",";

        for (uint8_t byte : record.payload) {
            file_ << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << std::dec << std::setfill(' ');
        }

        file_ << "\n";

        ++recordCount_;
        return true;
    } catch (const std::exception& e) {
        LOGE("Write CSV record failed: %s", e.what());
        return false;
    }
}

// CaptureManager
bool CaptureManager::startCapture(const std::string& directory, Format format) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (isCapturing_.load()) {
        LOGW("Capture already in progress");
        return false;
    }

    // Create directory if it doesn't exist
    std::filesystem::create_directories(directory);

    // Generate filename with timestamp
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&timeT);

    std::ostringstream oss;
    oss << directory << "/ca3_capture_"
        << std::put_time(&tm, "%Y%m%d_%H%M%S");

    switch (format) {
        case Format::BINARY:
            writer_ = std::make_unique<BinaryCaptureWriter>();
            oss << ".c3cap";
            break;
        case Format::JSONL:
            writer_ = std::make_unique<JsonlCaptureWriter>();
            oss << ".jsonl";
            break;
        case Format::CSV:
            writer_ = std::make_unique<CsvCaptureWriter>();
            oss << ".csv";
            break;
    }

    currentPath_ = oss.str();
    currentFormat_ = format;

    if (!writer_->open(currentPath_)) {
        writer_.reset();
        return false;
    }

    isCapturing_.store(true);
    recordCount_.store(0);
    LOGI("Capture started: %s", currentPath_.c_str());
    return true;
}

void CaptureManager::stopCapture() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (writer_) {
        writer_->close();
        writer_.reset();
    }
    isCapturing_.store(false);
    LOGI("Capture stopped, total records: %llu", recordCount_.load());
}

void CaptureManager::record(const CaptureRecord& record) {
    if (!isCapturing_.load() || !writer_) return;

    if (writer_->writeRecord(record)) {
        recordCount_.fetch_add(1);
    }
}

void CaptureManager::recordUsb(const uint8_t* data, size_t length, uint64_t timestampNs,
                               CaptureDirection direction, CaptureTransferType type,
                               uint8_t endpoint, uint8_t requestType, uint8_t request,
                               uint16_t value, uint16_t index) {
    if (!isCapturing_.load() || !writer_) return;

    CaptureRecord rec;
    rec.timestampNs = timestampNs;
    rec.direction = direction;
    rec.transferType = type;
    rec.endpoint = endpoint;
    rec.requestType = requestType;
    rec.request = request;
    rec.value = value;
    rec.index = index;
    rec.payload.assign(data, data + length);

    record(rec);
}

std::string CaptureManager::getCurrentCapturePath() const {
    return currentPath_;
}

} // namespace ca3