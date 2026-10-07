#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <optional>
#include <deque>

namespace ca3 {

// J1708 / J1587 Protocol Implementation
// SAE J1708 - Serial Data Communication Between Microcomputer Systems in Heavy Duty Vehicle Applications
// SAE J1587 - Electronic Data Interchange Between Microcomputer Systems in Heavy Duty Vehicle Applications

// J1708 frame structure
struct J1708Frame {
    uint8_t mid;              // Message ID (PID)
    uint8_t data[21];         // Up to 21 data bytes
    uint8_t dlc;              // Data length
    uint8_t checksum;         // 8-bit checksum
    uint64_t timestamp_ns;
    bool valid = false;
    
    // Calculate checksum (sum of all bytes including MID, modulo 256)
    static uint8_t calculateChecksum(uint8_t mid, const uint8_t* data, uint8_t dlc) {
        uint16_t sum = mid;
        for (uint8_t i = 0; i < dlc; ++i) {
            sum += data[i];
        }
        return static_cast<uint8_t>(sum & 0xFF);
    }
    
    bool verifyChecksum() const {
        return checksum == calculateChecksum(mid, data, dlc);
    }
};

// J1708 PID (Parameter ID) definitions per J1587
enum class J1708Pid : uint8_t {
    // Vehicle Identification
    VIN = 0x01,
    VEHICLE_ID = 0x02,
    ENGINE_SERIAL = 0x03,
    
    // Engine Parameters
    ENGINE_SPEED = 0x5C,        // 92 - Engine speed (rpm/16)
    ENGINE_TEMP = 0x5D,         // 93 - Engine coolant temperature
    OIL_PRESSURE = 0x5E,        // 94 - Oil pressure
    OIL_TEMP = 0x5F,            // 95 - Oil temperature
    FUEL_PRESSURE = 0x60,       // 96 - Fuel pressure
    BOOST_PRESSURE = 0x61,      // 97 - Boost pressure
    INTAKE_TEMP = 0x62,         // 98 - Intake manifold temperature
    EXHAUST_TEMP = 0x63,        // 99 - Exhaust temperature
    
    // Fuel
    FUEL_RATE = 0x64,           // 100 - Fuel rate
    FUEL_ECONOMY = 0x65,        // 101 - Instantaneous fuel economy
    FUEL_USED = 0x66,           // 102 - Trip fuel used
    TOTAL_FUEL = 0x67,          // 103 - Total fuel used
    
    // Vehicle
    VEHICLE_SPEED = 0x68,       // 104 - Vehicle speed
    TRIP_DISTANCE = 0x69,       // 105 - Trip distance
    TOTAL_DISTANCE = 0x6A,      // 106 - Total vehicle distance
    
    // Electrical
    BATTERY_VOLTAGE = 0x6B,     // 107 - Battery voltage
    ALTERNATOR_VOLTAGE = 0x6C,  // 108 - Alternator voltage
    
    // Transmission
    TRANS_TEMP = 0x6D,          // 109 - Transmission oil temperature
    TRANS_GEAR = 0x6E,          // 110 - Current gear
    
    // Brakes
    BRAKE_APP_PRESSURE = 0x6F,  // 111 - Brake application pressure
    BRAKE_PRIMARY = 0x70,       // 112 - Primary reservoir pressure
    BRAKE_SECONDARY = 0x71,     // 113 - Secondary reservoir pressure
    
    // Diagnostic
    ACTIVE_CODES = 0x72,        // 114 - Active diagnostic codes
    PREVIOUS_CODES = 0x73,      // 115 - Previous diagnostic codes
    CLEAR_CODES = 0x74,         // 116 - Clear diagnostic codes
    
    // Proprietary
    PROPRIETARY_START = 0xE0,
    PROPRIETARY_END = 0xFF
};

// J1587 PID data formats
struct J1587Parameter {
    J1708Pid pid;
    std::string name;
    std::string description;
    std::string units;
    double resolution;
    double offset;
    uint8_t data_bytes;
    bool signed_value;
    
    double decode(const uint8_t* data) const {
        if (!data) return 0.0;
        
        uint32_t raw = 0;
        for (int i = 0; i < data_bytes; ++i) {
            raw |= (data[i] << (i * 8));
        }
        
        double value;
        if (signed_value) {
            // Sign extend
            int32_t signed_raw = static_cast<int32_t>(raw << ((4 - data_bytes) * 8)) >> ((4 - data_bytes) * 8);
            value = signed_raw * resolution + offset;
        } else {
            value = raw * resolution + offset;
        }
        return value;
    }
};

// Known J1587 parameters
static const std::vector<J1587Parameter> J1587_PARAMETERS = {
    {J1708Pid::ENGINE_SPEED, "Engine Speed", "Engine speed", "rpm", 16.0, 0.0, 2, false},
    {J1708Pid::ENGINE_TEMP, "Engine Coolant Temperature", "Engine coolant temperature", "°C", 1.0, -40.0, 1, true},
    {J1708Pid::OIL_PRESSURE, "Engine Oil Pressure", "Engine oil pressure", "kPa", 4.0, 0.0, 1, false},
    {J1708Pid::OIL_TEMP, "Engine Oil Temperature", "Engine oil temperature", "°C", 1.0, -40.0, 1, true},
    {J1708Pid::FUEL_PRESSURE, "Fuel Pressure", "Fuel pressure", "kPa", 4.0, 0.0, 1, false},
    {J1708Pid::BOOST_PRESSURE, "Boost Pressure", "Boost pressure", "kPa", 4.0, 0.0, 1, false},
    {J1708Pid::INTAKE_TEMP, "Intake Manifold Temperature", "Intake manifold temperature", "°C", 1.0, -40.0, 1, true},
    {J1708Pid::EXHAUST_TEMP, "Exhaust Temperature", "Exhaust gas temperature", "°C", 1.0, -40.0, 2, true},
    {J1708Pid::FUEL_RATE, "Fuel Rate", "Fuel consumption rate", "L/h", 0.05, 0.0, 2, false},
    {J1708Pid::FUEL_ECONOMY, "Instantaneous Fuel Economy", "Fuel economy", "km/L", 0.002, 0.0, 2, false},
    {J1708Pid::FUEL_USED, "Trip Fuel Used", "Trip fuel consumed", "L", 0.5, 0.0, 2, false},
    {J1708Pid::TOTAL_FUEL, "Total Fuel Used", "Total fuel consumed", "L", 0.5, 0.0, 3, false},
    {J1708Pid::VEHICLE_SPEED, "Vehicle Speed", "Vehicle speed", "km/h", 1.0/256.0, 0.0, 2, false},
    {J1708Pid::TRIP_DISTANCE, "Trip Distance", "Trip distance", "km", 0.125, 0.0, 3, false},
    {J1708Pid::TOTAL_DISTANCE, "Total Vehicle Distance", "Total vehicle distance", "km", 0.125, 0.0, 4, false},
    {J1708Pid::BATTERY_VOLTAGE, "Battery Voltage", "Battery potential", "V", 0.05, 0.0, 1, false},
    {J1708Pid::ALTERNATOR_VOLTAGE, "Alternator Voltage", "Alternator potential", "V", 0.05, 0.0, 1, false},
    {J1708Pid::TRANS_TEMP, "Transmission Oil Temperature", "Transmission oil temperature", "°C", 1.0, -40.0, 1, true},
    {J1708Pid::TRANS_GEAR, "Current Gear", "Current transmission gear", "", 1.0, 0.0, 1, false},
    {J1708Pid::BRAKE_APP_PRESSURE, "Brake Application Pressure", "Brake application pressure", "kPa", 4.0, 0.0, 1, false},
    {J1708Pid::BRAKE_PRIMARY, "Primary Reservoir Pressure", "Primary air reservoir pressure", "kPa", 4.0, 0.0, 1, false},
    {J1708Pid::BRAKE_SECONDARY, "Secondary Reservoir Pressure", "Secondary air reservoir pressure", "kPa", 4.0, 0.0, 1, false}
};

// J1708 decoder
class J1708Decoder {
public:
    struct DecodeResult {
        J1708Frame frame;
        std::vector<std::pair<std::string, double>> parameters;
        std::string error;
    };
    
    J1708Decoder() = default;
    
    // Feed raw bytes - returns complete frames when available
    std::vector<DecodeResult> feed(const uint8_t* data, size_t len, uint64_t timestamp_ns) {
        std::vector<DecodeResult> results;
        
        for (size_t i = 0; i < len; ++i) {
            buffer_.push_back({data[i], timestamp_ns});
            
            // Try to extract frames
            while (tryExtractFrame(results, timestamp_ns)) {
                // Continue extracting
            }
            
            // Prevent buffer overflow
            if (buffer_.size() > 1024) {
                buffer_.erase(buffer_.begin(), buffer_.begin() + 512);
            }
        }
        
        return results;
    }
    
    void reset() {
        buffer_.clear();
    }

private:
    struct TimedByte {
        uint8_t byte;
        uint64_t timestamp;
    };
    
    std::deque<TimedByte> buffer_;
    
    // J1708 uses bit-stuffing and specific framing
    // Frame format: [MID] [DATA...] [CHECKSUM]
    // Frames are separated by bus idle time (>= 1 bit time)
    // In USB capture, we look for MID bytes and checksum validation
    
    bool tryExtractFrame(std::vector<DecodeResult>& results, uint64_t current_time) {
        if (buffer_.size() < 3) return false; // Minimum: MID + 1 data + checksum
        
        // Look for potential MID bytes (0x01-0xFF, but certain ranges reserved)
        // J1708 MID 0x00 is invalid
        
        for (size_t i = 0; i + 2 < buffer_.size(); ++i) {
            uint8_t potential_mid = buffer_[i].byte;
            
            // Valid MID range
            if (potential_mid == 0x00) continue;
            
            // Try different data lengths
            for (uint8_t dlc = 1; dlc <= 21 && i + 1 + dlc < buffer_.size(); ++dlc) {
                size_t checksum_pos = i + 1 + dlc;
                if (checksum_pos >= buffer_.size()) break;
                
                uint8_t checksum = buffer_[checksum_pos].byte;
                uint8_t calc = J1708Frame::calculateChecksum(potential_mid, 
                    &buffer_[i + 1].byte, dlc);
                
                if (checksum == calc) {
                    // Found valid frame!
                    J1708Frame frame;
                    frame.mid = potential_mid;
                    frame.dlc = dlc;
                    frame.checksum = checksum;
                    frame.timestamp_ns = buffer_[i].timestamp;
                    frame.valid = true;
                    
                    for (uint8_t j = 0; j < dlc; ++j) {
                        frame.data[j] = buffer_[i + 1 + j].byte;
                    }
                    
                    DecodeResult result;
                    result.frame = frame;
                    result.error = "";
                    
                    // Decode known parameters
                    decodeParameters(frame, result.parameters);
                    
                    results.push_back(std::move(result));
                    
                    // Remove processed bytes from buffer
                    buffer_.erase(buffer_.begin(), buffer_.begin() + checksum_pos + 1);
                    return true; // Continue checking
                }
            }
        }
        
        return false;
    }
    
    void decodeParameters(const J1708Frame& frame, std::vector<std::pair<std::string, double>>& params) {
        for (const auto& param : J1587_PARAMETERS) {
            if (param.pid == static_cast<J1708Pid>(frame.mid) && frame.dlc >= param.data_bytes) {
                params.emplace_back(param.name, param.decode(frame.data));
            }
        }
        
        // Also check for multi-parameter PIDs (PID with sub-parameters)
        if (frame.dlc > 0) {
            // Some PIDs contain multiple parameters packed
            // This requires PID-specific knowledge
        }
    }
};

// J1708 bus analyzer
class J1708Analyzer {
public:
    struct Stats {
        size_t frames_received = 0;
        size_t checksum_errors = 0;
        size_t bytes_processed = 0;
    };
    
    void feed(const uint8_t* data, size_t len, uint64_t timestamp_ns) {
        auto results = decoder_.feed(data, len, timestamp_ns);
        for (auto& result : results) {
            if (result.frame.valid) {
                stats_.frames_received++;
                frames_.push_back(result);
                if (frames_.size() > 10000) frames_.pop_front();
            } else {
                stats_.checksum_errors++;
            }
        }
        stats_.bytes_processed += len;
    }
    
    const std::deque<J1708Decoder::DecodeResult>& getFrames() const { return frames_; }
    const Stats& getStats() const { return stats_; }
    void reset() { decoder_.reset(); frames_.clear(); stats_ = {}; }

private:
    J1708Decoder decoder_;
    std::deque<J1708Decoder::DecodeResult> frames_;
    Stats stats_;
};

} // namespace ca3