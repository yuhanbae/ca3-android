#pragma once

#include <cstdint>
#include <vector>
#include <cstring>
#include <algorithm>

namespace ca3 {

class UsbBuffer {
public:
    UsbBuffer(size_t initialCapacity = 4096) {
        buffer_.reserve(initialCapacity);
    }

    void clear() {
        readPos_ = 0;
        writePos_ = 0;
    }

    size_t size() const {
        return writePos_ - readPos_;
    }

    size_t capacity() const {
        return buffer_.size();
    }

    size_t available() const {
        return buffer_.capacity() - writePos_;
    }

    bool empty() const {
        return readPos_ == writePos_;
    }

    // Write data to buffer
    size_t write(const uint8_t* data, size_t length) {
        ensureCapacity(length);
        size_t written = std::min(length, available());
        std::memcpy(buffer_.data() + writePos_, data, written);
        writePos_ += written;
        return written;
    }

    // Read data from buffer
    size_t read(uint8_t* data, size_t length) {
        size_t available = size();
        size_t toRead = std::min(length, available);
        if (toRead > 0) {
            std::memcpy(data, buffer_.data() + readPos_, toRead);
            readPos_ += toRead;
            // Compact buffer if read position gets too large
            if (readPos_ > buffer_.capacity() / 2) {
                compact();
            }
        }
        return toRead;
    }

    // Peek at data without removing it
    size_t peek(uint8_t* data, size_t length) const {
        size_t available = size();
        size_t toPeek = std::min(length, available);
        if (toPeek > 0) {
            std::memcpy(data, buffer_.data() + readPos_, toPeek);
        }
        return toPeek;
    }

    // Get pointer to readable data
    const uint8_t* data() const {
        return buffer_.data() + readPos_;
    }

    // Get pointer to writable space
    uint8_t* writePtr() {
        return buffer_.data() + writePos_;
    }

    // Advance write position (after direct write to writePtr)
    void advanceWrite(size_t length) {
        writePos_ += length;
    }

    // Advance read position (after consuming data)
    void advanceRead(size_t length) {
        readPos_ = std::min(readPos_ + length, writePos_);
    }

    // Find byte sequence in buffer
    ssize_t find(const uint8_t* pattern, size_t patternLen) const {
        if (patternLen == 0 || size() < patternLen) return -1;

        const uint8_t* buf = data();
        size_t len = size();

        for (size_t i = 0; i <= len - patternLen; ++i) {
            if (std::memcmp(buf + i, pattern, patternLen) == 0) {
                return static_cast<ssize_t>(i);
            }
        }
        return -1;
    }

    // Find byte sequence and remove data up to and including it
    bool findAndConsume(const uint8_t* pattern, size_t patternLen) {
        ssize_t pos = find(pattern, patternLen);
        if (pos >= 0) {
            advanceRead(static_cast<size_t>(pos) + patternLen);
            return true;
        }
        return false;
    }

    // Resize buffer if needed
    void reserve(size_t newCapacity) {
        if (newCapacity > buffer_.capacity()) {
            buffer_.reserve(newCapacity);
        }
    }

private:
    void ensureCapacity(size_t needed) {
        if (available() < needed) {
            // Try to compact first
            if (readPos_ > 0) {
                compact();
            }
            // If still not enough, grow buffer
            if (available() < needed) {
                size_t newCap = std::max(buffer_.capacity() * 2, writePos_ + needed);
                buffer_.reserve(newCap);
            }
        }
    }

    void compact() {
        if (readPos_ > 0 && readPos_ < writePos_) {
            size_t len = size();
            std::memmove(buffer_.data(), buffer_.data() + readPos_, len);
            readPos_ = 0;
            writePos_ = len;
        }
    }

    std::vector<uint8_t> buffer_;
    size_t readPos_ = 0;
    size_t writePos_ = 0;
};

// Double buffer for ping-pong reading
class DoubleBuffer {
public:
    DoubleBuffer(size_t bufferSize = 4096) : active_(0) {
        buffers_[0].reserve(bufferSize);
        buffers_[1].reserve(bufferSize);
    }

    UsbBuffer& getActive() { return buffers_[active_]; }
    UsbBuffer& getInactive() { return buffers_[1 - active_]; }

    void swap() {
        active_ = 1 - active_;
    }

    void clear() {
        buffers_[0].clear();
        buffers_[1].clear();
        active_ = 0;
    }

private:
    UsbBuffer buffers_[2];
    int active_;
};

} // namespace ca3