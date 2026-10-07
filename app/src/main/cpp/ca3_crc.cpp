#include "ca3_crc.hpp"
#include <algorithm>

namespace ca3 {

// CRC-8 table
static uint8_t crc8_table[256];

static void init_crc8_table(uint8_t poly) {
    for (int i = 0; i < 256; i++) {
        uint8_t crc = i;
        for (int j = 0; j < 8; j++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ poly) : (uint8_t)(crc << 1);
        }
        crc8_table[i] = crc;
    }
}

uint8_t CrcCalculator::crc8(const uint8_t* data, size_t len, uint8_t poly, uint8_t init, bool refl_in, bool refl_out, uint8_t xor_out) {
    static bool table_init = false;
    if (!table_init) {
        init_crc8_table(poly);
        table_init = true;
    }
    
    uint8_t crc = init;
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        if (refl_in) {
            // Reflect byte
            byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
            byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
            byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
        }
        crc = crc8_table[crc ^ byte];
    }
    if (refl_out) {
        crc = (crc & 0xF0) >> 4 | (crc & 0x0F) << 4;
        crc = (crc & 0xCC) >> 2 | (crc & 0x33) << 2;
        crc = (crc & 0xAA) >> 1 | (crc & 0x55) << 1;
    }
    return crc ^ xor_out;
}

uint8_t CrcCalculator::crc8_maxim(const uint8_t* data, size_t len) {
    return crc8(data, len, 0x31, 0x00, true, true, 0x00);
}

uint8_t CrcCalculator::crc8_rohc(const uint8_t* data, size_t len) {
    return crc8(data, len, 0x07, 0xFF, true, true, 0x00);
}

// CRC-16 table
static uint16_t crc16_table[256];

static void init_crc16_table(uint16_t poly) {
    for (int i = 0; i < 256; i++) {
        uint16_t crc = i << 8;
        for (int j = 0; j < 8; j++) {
            crc = (crc & 0x8000) ? (crc << 1) ^ poly : (crc << 1);
        }
        crc16_table[i] = crc;
    }
}

uint16_t CrcCalculator::crc16(const uint8_t* data, size_t len, uint16_t poly, uint16_t init, bool refl_in, bool refl_out, uint16_t xor_out) {
    static bool table_init = false;
    if (!table_init) {
        init_crc16_table(poly);
        table_init = true;
    }
    
    uint16_t crc = init;
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        if (refl_in) {
            // Reflect byte
            byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
            byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
            byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
        }
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ byte) & 0xFF];
    }
    if (refl_out) {
        // Reflect 16-bit
        crc = ((crc & 0xFF00) >> 8) | ((crc & 0x00FF) << 8);
        crc = ((crc & 0xF0F0) >> 4) | ((crc & 0x0F0F) << 4);
        crc = ((crc & 0xCCCC) >> 2) | ((crc & 0x3333) << 2);
        crc = ((crc & 0xAAAA) >> 1) | ((crc & 0x5555) << 1);
    }
    return crc ^ xor_out;
}

uint16_t CrcCalculator::crc16_ccitt(const uint8_t* data, size_t len) {
    return crc16(data, len, 0x1021, 0xFFFF, false, false, 0x0000);
}

uint16_t CrcCalculator::crc16_xmodem(const uint8_t* data, size_t len) {
    return crc16(data, len, 0x1021, 0x0000, false, false, 0x0000);
}

uint16_t CrcCalculator::crc16_modbus(const uint8_t* data, size_t len) {
    return crc16(data, len, 0x8005, 0xFFFF, true, true, 0x0000);
}

uint16_t CrcCalculator::crc16_kermit(const uint8_t* data, size_t len) {
    return crc16(data, len, 0x1021, 0x0000, true, true, 0x0000);
}

// CRC-32 table
static uint32_t crc32_table[256];
static bool crc32_table_init = false;

static void init_crc32_table(uint32_t poly) {
    for (int i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            crc = (crc & 1) ? (crc >> 1) ^ poly : (crc >> 1);
        }
        crc32_table[i] = crc;
    }
    crc32_table_init = true;
}

uint32_t CrcCalculator::crc32(const uint8_t* data, size_t len, uint32_t poly, uint32_t init, bool refl_in, bool refl_out, uint32_t xor_out) {
    if (!crc32_table_init) {
        init_crc32_table(poly);
    }
    
    uint32_t crc = init;
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        if (refl_in) {
            // Reflect byte
            byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
            byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
            byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
        }
        crc = (crc >> 8) ^ crc32_table[(crc ^ byte) & 0xFF];
    }
    if (refl_out) {
        // Reflect 32-bit
        crc = ((crc & 0xFF000000) >> 24) | ((crc & 0x00FF0000) >> 8) | 
              ((crc & 0x0000FF00) << 8) | ((crc & 0x000000FF) << 24);
        crc = ((crc & 0xF0F0F0F0) >> 4) | ((crc & 0x0F0F0F0F) << 4);
        crc = ((crc & 0xCCCCCCCC) >> 2) | ((crc & 0x33333333) << 2);
        crc = ((crc & 0xAAAAAAAA) >> 1) | ((crc & 0x55555555) << 1);
    }
    return crc ^ xor_out;
}

uint32_t CrcCalculator::crc32_mpeg2(const uint8_t* data, size_t len) {
    return crc32(data, len, 0x04C11DB7, 0xFFFFFFFF, false, false, 0x00000000);
}

uint32_t CrcCalculator::crc32_cksum(const uint8_t* data, size_t len) {
    return crc32(data, len, 0x04C11DB7, 0x00000000, false, false, 0xFFFFFFFF);
}

std::vector<CrcCalculator::CrcResult> CrcCalculator::testAll(const uint8_t* data, size_t len, uint64_t expected_crc) {
    std::vector<CrcResult> results;
    
    // CRC-8 variants
    auto add8 = [&](const char* name, uint8_t value) {
        results.push_back({name, value, value == (expected_crc & 0xFF)});
    };
    
    add8("CRC-8 (poly=0x07)", crc8(data, len, 0x07));
    add8("CRC-8 MAXIM", crc8_maxim(data, len));
    add8("CRC-8 ROHC", crc8_rohc(data, len));
    add8("CRC-8 (poly=0x1D)", crc8(data, len, 0x1D));
    add8("CRC-8 (poly=0x31)", crc8(data, len, 0x31));
    
    // CRC-16 variants
    auto add16 = [&](const char* name, uint16_t value) {
        results.push_back({name, value, value == (expected_crc & 0xFFFF)});
    };
    
    add16("CRC-16 CCITT", crc16_ccitt(data, len));
    add16("CRC-16 XMODEM", crc16_xmodem(data, len));
    add16("CRC-16 MODBUS", crc16_modbus(data, len));
    add16("CRC-16 KERMIT", crc16_kermit(data, len));
    add16("CRC-16 (poly=0x8005, init=0)", crc16(data, len, 0x8005, 0x0000));
    add16("CRC-16 (poly=0x1021, init=0)", crc16(data, len, 0x1021, 0x0000));
    
    // CRC-32 variants
    auto add32 = [&](const char* name, uint32_t value) {
        results.push_back({name, value, value == (expected_crc & 0xFFFFFFFF)});
    };
    
    add32("CRC-32 (ISO)", crc32(data, len));
    add32("CRC-32 MPEG-2", crc32_mpeg2(data, len));
    add32("CRC-32 CKSUM", crc32_cksum(data, len));
    add32("CRC-32 (poly=0x1EDC6F41)", crc32(data, len, 0x1EDC6F41));
    
    return results;
}

} // namespace ca3