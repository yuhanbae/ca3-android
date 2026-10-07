#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <array>
#include <optional>

namespace ca3 {

// CRC algorithms for protocol analysis
class CrcCalculator {
public:
    // CRC-8 variants
    static uint8_t crc8(const uint8_t* data, size_t len, uint8_t poly = 0x07, uint8_t init = 0x00, bool refl_in = false, bool refl_out = false, uint8_t xor_out = 0x00);
    static uint8_t crc8_maxim(const uint8_t* data, size_t len);
    static uint8_t crc8_rohc(const uint8_t* data, size_t len);
    
    // CRC-16 variants
    static uint16_t crc16(const uint8_t* data, size_t len, uint16_t poly = 0x8005, uint16_t init = 0xFFFF, bool refl_in = true, bool refl_out = true, uint16_t xor_out = 0x0000);
    static uint16_t crc16_ccitt(const uint8_t* data, size_t len);
    static uint16_t crc16_xmodem(const uint8_t* data, size_t len);
    static uint16_t crc16_modbus(const uint8_t* data, size_t len);
    static uint16_t crc16_kermit(const uint8_t* data, size_t len);
    
    // CRC-32 variants
    static uint32_t crc32(const uint8_t* data, size_t len, uint32_t poly = 0x04C11DB7, uint32_t init = 0xFFFFFFFF, bool refl_in = true, bool refl_out = true, uint32_t xor_out = 0xFFFFFFFF);
    static uint32_t crc32_mpeg2(const uint8_t* data, size_t len);
    static uint32_t crc32_cksum(const uint8_t* data, size_t len);
    
    // Test all common CRC variants against data
    struct CrcResult {
        std::string name;
        uint64_t value;
        bool matches;
    };
    
    static std::vector<CrcResult> testAll(const uint8_t* data, size_t len, uint64_t expected_crc);
};

} // namespace ca3