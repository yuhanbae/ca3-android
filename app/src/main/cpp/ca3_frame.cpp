#include "ca3_frame.hpp"
#include "ca3_crc.hpp"
#include <algorithm>
#include <iostream>

namespace ca3 {

FrameAnalyzer::FrameAnalyzer(Config config) : config_(config) {
    buffer_.reserve(config_.max_buffer_size);
}

void FrameAnalyzer::feed(const uint8_t* data, size_t len) {
    if (buffer_.size() + len > config_.max_buffer_size) {
        // Remove oldest data to make room
        size_t remove = (buffer_.size() + len) - config_.max_buffer_size;
        buffer_.erase(buffer_.begin(), buffer_.begin() + remove);
    }
    buffer_.insert(buffer_.end(), data, data + len);
}

std::vector<FrameCandidate> FrameAnalyzer::findCandidates() {
    std::vector<FrameCandidate> candidates;
    
    if (buffer_.size() < config_.min_frame_size) {
        return candidates;
    }
    
    for (size_t i = 0; i + config_.min_frame_size <= buffer_.size(); ++i) {
        // Skip if start byte doesn't match (if specified)
        if (config_.start_byte != 0 && buffer_[i] != config_.start_byte) {
            continue;
        }
        
        // Try different frame formats
        if (auto c = tryLengthPrefixedFrame(i)) candidates.push_back(*c);
        if (auto c = tryDelimitedFrame(i)) candidates.push_back(*c);
        if (auto c = tryFixedLengthFrame(i, buffer_[i + 1])) candidates.push_back(*c);
        if (auto c = tryCrcFrame(i, buffer_.size() - i)) candidates.push_back(*c);
    }
    
    // Sort by confidence
    std::sort(candidates.begin(), candidates.end(),
        [](const FrameCandidate& a, const FrameCandidate& b) {
            return a.confidence > b.confidence;
        });
    
    return candidates;
}

std::optional<FrameCandidate> FrameAnalyzer::tryLengthPrefixedFrame(size_t start) {
    if (start + 2 > buffer_.size()) return std::nullopt;
    
    uint8_t length_byte = buffer_[start + 1];
    size_t frame_len = length_byte + 2; // length byte + payload + length byte itself
    
    if (frame_len < config_.min_frame_size || frame_len > config_.max_frame_size) {
        return std::nullopt;
    }
    
    if (start + frame_len > buffer_.size()) {
        return std::nullopt; // Incomplete frame
    }
    
    // Check CRC if present
    bool has_crc = false;
    std::string crc_type;
    if (frame_len >= 4) {
        auto crc_results = CrcCalculator::testAll(
            &buffer_[start], frame_len - 2,
            buffer_[start + frame_len - 2] | (buffer_[start + frame_len - 1] << 8)
        );
        
        for (const auto& r : crc_results) {
            if (r.matches) {
                has_crc = true;
                crc_type = r.name;
                break;
            }
        }
    }
    
    FrameCandidate candidate;
    candidate.offset = start;
    candidate.length = frame_len;
    candidate.possible_start_byte = buffer_[start];
    candidate.possible_length_byte = length_byte;
    candidate.has_valid_crc = has_crc;
    candidate.crc_type = crc_type;
    candidate.confidence = has_crc ? 0.9 : 0.6;
    candidate.hypothesis = "Length-prefixed frame (length byte at offset 1)";
    
    if (hypothesis_callback_) hypothesis_callback_(candidate);
    return candidate;
}

std::optional<FrameCandidate> FrameAnalyzer::tryDelimitedFrame(size_t start) {
    // Look for start/end delimiters (e.g., 0x7E, 0x55, 0xAA)
    static const uint8_t common_delimiters[] = {0x7E, 0x55, 0xAA, 0x5A, 0xA5, 0x02, 0x03};
    
    uint8_t start_delim = buffer_[start];
    bool is_common = false;
    for (uint8_t d : common_delimiters) if (start_delim == d) { is_common = true; break; }
    
    // Search for end delimiter
    for (size_t i = start + 1; i < buffer_.size() && i < start + config_.max_frame_size; ++i) {
        if (buffer_[i] == start_delim || buffer_[i] == 0x7E) { // Common end delimiters
            size_t frame_len = i - start + 1;
            if (frame_len >= config_.min_frame_size) {
                FrameCandidate candidate;
                candidate.offset = start;
                candidate.length = frame_len;
                candidate.possible_start_byte = start_delim;
                candidate.possible_length_byte = 0;
                candidate.has_valid_crc = false;
                candidate.crc_type = "";
                candidate.confidence = is_common ? 0.5 : 0.3;
                candidate.hypothesis = "Delimited frame (start=0x" + 
                    std::to_string(start_delim) + ", end=0x" + std::to_string(buffer_[i]) + ")";
                if (hypothesis_callback_) hypothesis_callback_(candidate);
                return candidate;
            }
        }
    }
    return std::nullopt;
}

std::optional<FrameCandidate> FrameAnalyzer::tryFixedLengthFrame(size_t start, uint8_t length_byte) {
    size_t frame_len = length_byte;
    if (frame_len < config_.min_frame_size || frame_len > config_.max_frame_size) {
        return std::nullopt;
    }
    if (start + frame_len > buffer_.size()) return std::nullopt;
    
    FrameCandidate candidate;
    candidate.offset = start;
    candidate.length = frame_len;
    candidate.possible_start_byte = buffer_[start];
    candidate.possible_length_byte = length_byte;
    candidate.has_valid_crc = false;
    candidate.crc_type = "";
    candidate.confidence = 0.4;
    candidate.hypothesis = "Fixed-length frame (length=0x" + std::to_string(length_byte) + ")";
    if (hypothesis_callback_) hypothesis_callback_(candidate);
    return candidate;
}

std::optional<FrameCandidate> FrameAnalyzer::tryCrcFrame(size_t start, size_t max_len) {
    // Try frames ending with CRC
    for (size_t len = config_.min_frame_size; len <= std::min(max_len, config_.max_frame_size); ++len) {
        if (len < 3) continue;
        
        // Try CRC-16 at end
        if (len >= 3) {
            uint16_t expected = buffer_[start + len - 2] | (buffer_[start + len - 1] << 8);
            auto results = CrcCalculator::testAll(&buffer_[start], len - 2, expected);
            for (const auto& r : results) {
                if (r.matches) {
                    FrameCandidate candidate;
                    candidate.offset = start;
                    candidate.length = len;
                    candidate.possible_start_byte = buffer_[start];
                    candidate.possible_length_byte = 0;
                    candidate.has_valid_crc = true;
                    candidate.crc_type = r.name;
                    candidate.confidence = 0.95;
                    candidate.hypothesis = "Frame with valid " + r.name + " at end";
                    if (hypothesis_callback_) hypothesis_callback_(candidate);
                    return candidate;
                }
            }
        }
        
        // Try CRC-8 at end
        if (len >= 2) {
            uint8_t expected = buffer_[start + len - 1];
            auto results = CrcCalculator::testAll(&buffer_[start], len - 1, expected);
            for (const auto& r : results) {
                if (r.matches) {
                    FrameCandidate candidate;
                    candidate.offset = start;
                    candidate.length = len;
                    candidate.possible_start_byte = buffer_[start];
                    candidate.possible_length_byte = 0;
                    candidate.has_valid_crc = true;
                    candidate.crc_type = r.name;
                    candidate.confidence = 0.9;
                    candidate.hypothesis = "Frame with valid " + r.name + " at end";
                    if (hypothesis_callback_) hypothesis_callback_(candidate);
                    return candidate;
                }
            }
        }
    }
    return std::nullopt;
}

bool FrameAnalyzer::checkRepeatedHeader(const uint8_t* data, size_t len, size_t header_len) {
    if (len < header_len * 2) return false;
    for (size_t i = 0; i < header_len; ++i) {
        if (data[i] != data[i + header_len]) return false;
    }
    return true;
}

void FrameAnalyzer::detectFieldPatterns(const uint8_t* data, size_t len, FrameAnalysis& analysis) {
    // Look for length field (usually at offset 1 or 2)
    if (len >= 3) {
        // Check if byte at offset 1 matches remaining length
        if (data[1] == len - 2) {
            analysis.length_field_pos = 1;
            analysis.hypotheses.push_back("Length field at offset 1 (value=" + std::to_string(data[1]) + ")");
        }
        if (data[2] == len - 3) {
            analysis.length_field_pos = 2;
            analysis.hypotheses.push_back("Length field at offset 2 (value=" + std::to_string(data[2]) + ")");
        }
    }
    
    // Look for sequence number (incrementing)
    if (len >= 2) {
        // Would need multiple frames to confirm
        analysis.hypotheses.push_back("Possible sequence field at offset 1 (value=0x" + 
            std::to_string(data[1]) + ")");
    }
    
    // Look for command field (often at offset 1 or after length)
    if (len >= 3) {
        analysis.cmd_field_pos = 2;
        analysis.hypotheses.push_back("Possible command field at offset 2 (value=0x" + 
            std::to_string(data[2]) + ")");
    }
    
    // Check for repeated patterns across buffer
    // This would require multiple frames
}

FrameAnalyzer::FrameAnalysis FrameAnalyzer::analyzeFrame(const uint8_t* data, size_t len) {
    FrameAnalysis analysis;
    analysis.payload.assign(data, data + len);
    
    // Basic sanity
    if (len < config_.min_frame_size) {
        analysis.hypotheses.push_back("Frame too short (" + std::to_string(len) + " bytes)");
        return analysis;
    }
    
    // Detect field patterns
    detectFieldPatterns(data, len, analysis);
    
    // Check for CRC
    if (len >= 3) {
        // CRC-16 at end
        uint16_t expected16 = data[len - 2] | (data[len - 1] << 8);
        auto results16 = CrcCalculator::testAll(data, len - 2, expected16);
        for (const auto& r : results16) {
            if (r.matches) {
                analysis.crc_info = {len - 2, r.name};
                analysis.hypotheses.push_back("Valid " + r.name + " at end (offset " + std::to_string(len - 2) + ")");
            }
        }
    }
    
    if (len >= 2) {
        // CRC-8 at end
        uint8_t expected8 = data[len - 1];
        auto results8 = CrcCalculator::testAll(data, len - 1, expected8);
        for (const auto& r : results8) {
            if (r.matches) {
                analysis.crc_info = {len - 1, r.name};
                analysis.hypotheses.push_back("Valid " + r.name + " at end (offset " + std::to_string(len - 1) + ")");
            }
        }
    }
    
    // Check for common start bytes
    static const std::pair<uint8_t, const char*> known_starts[] = {
        {0x55, "KWP2000/ISO14230"}, {0xAA, "CAN/ISO-TP"}, {0x7E, "HDLC/PPP"},
        {0x02, "STX"}, {0x03, "ETX"}, {0x5A, "J1939"}, {0xA5, "Custom"}
    };
    
    for (auto [byte, proto] : known_starts) {
        if (data[0] == byte) {
            analysis.hypotheses.push_back("Start byte 0x" + std::to_string(byte) + " matches " + proto);
        }
    }
    
    return analysis;
}

void FrameAnalyzer::clear() {
    buffer_.clear();
}

size_t FrameAnalyzer::bufferSize() const {
    return buffer_.size();
}

void FrameAnalyzer::setHypothesisCallback(std::function<void(const FrameCandidate&)> callback) {
    hypothesis_callback_ = callback;
}

} // namespace ca3