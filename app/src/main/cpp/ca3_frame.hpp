#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <optional>
#include <functional>
#include <deque>

namespace ca3 {

// Frame candidate detection for unknown protocols
struct FrameCandidate {
    size_t offset;
    size_t length;
    uint8_t possible_start_byte;
    uint8_t possible_length_byte;
    bool has_valid_crc;
    std::string crc_type;
    double confidence;
    std::string hypothesis;
};

// Frame analyzer for protocol reverse engineering
class FrameAnalyzer {
public:
    struct Config {
        size_t min_frame_size = 4;
        size_t max_frame_size = 4096;
        uint8_t start_byte = 0;  // 0 = any
        bool require_crc = false;
        size_t max_buffer_size = 64 * 1024;
    };
    
    explicit FrameAnalyzer(Config config = {});
    
    // Feed raw bytes into the analyzer
    void feed(const uint8_t* data, size_t len);
    
    // Find frame candidates in current buffer
    std::vector<FrameCandidate> findCandidates();
    
    // Analyze a specific frame for patterns
    struct FrameAnalysis {
        std::vector<uint8_t> payload;
        std::vector<std::string> hypotheses;
        std::optional<uint8_t> length_field_pos;
        std::optional<uint8_t> seq_field_pos;
        std::optional<uint8_t> cmd_field_pos;
        std::optional<std::pair<size_t, std::string>> crc_info;
    };
    
    FrameAnalysis analyzeFrame(const uint8_t* data, size_t len);
    
    // Clear internal buffer
    void clear();
    
    // Get current buffer size
    size_t bufferSize() const;
    
    // Set hypothesis callback
    void setHypothesisCallback(std::function<void(const FrameCandidate&)> callback);

private:
    Config config_;
    std::vector<uint8_t> buffer_;
    std::function<void(const FrameCandidate&)> hypothesis_callback_;
    
    // Pattern detection helpers
    std::optional<FrameCandidate> tryFixedLengthFrame(size_t start, uint8_t length_byte);
    std::optional<FrameCandidate> tryDelimitedFrame(size_t start);
    std::optional<FrameCandidate> tryLengthPrefixedFrame(size_t start);
    std::optional<FrameCandidate> tryCrcFrame(size_t start, size_t len);
    bool checkRepeatedHeader(const uint8_t* data, size_t len, size_t header_len);
    void detectFieldPatterns(const uint8_t* data, size_t len, FrameAnalysis& analysis);
};

} // namespace ca3