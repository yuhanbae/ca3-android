#include "cat.hpp"
#include <android/log.h>

#define LOG_TAG "CAT"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace ca3 {

CatProtocol::CatProtocol() = default;

CatProtocol::~CatProtocol() = default;

// Static parameter database definition
std::unordered_map<uint16_t, CatParameter> CatProtocol::parameterDatabase_ = {
    // Engine parameters
    {0xF004, {0xF004, "Engine Speed", "Engine speed", "rpm", 0.125, 0.0, 2}},
    {0x005C, {0x005C, "Engine Speed (alt)", "Engine speed", "rpm", 1.0, 0.0, 2}},
    {0x005D, {0x005D, "Engine Coolant Temperature", "Engine coolant temperature", "°C", 1.0, -40.0, 1}},
    {0x005E, {0x005E, "Engine Oil Pressure", "Engine oil pressure", "kPa", 4.0, 0.0, 1}},
    {0x005F, {0x005F, "Engine Oil Temperature", "Engine oil temperature", "°C", 1.0, -40.0, 1}},
    {0x0060, {0x0060, "Fuel Pressure", "Fuel pressure", "kPa", 4.0, 0.0, 1}},
    {0x0061, {0x0061, "Boost Pressure", "Boost pressure", "kPa", 4.0, 0.0, 1}},
    {0x0062, {0x0062, "Intake Manifold Temperature", "Intake manifold temperature", "°C", 1.0, -40.0, 1}},
    {0x0063, {0x0063, "Exhaust Temperature", "Exhaust temperature", "°C", 1.0, -40.0, 2}},
    {0x0064, {0x0064, "Fuel Rate", "Fuel rate", "L/h", 0.05, 0.0, 2}},
    {0x0065, {0x0065, "Instantaneous Fuel Economy", "Fuel economy", "km/L", 0.002, 0.0, 2}},
    {0x0066, {0x0066, "Trip Fuel Used", "Trip fuel used", "L", 0.5, 0.0, 2}},
    {0x0067, {0x0067, "Total Fuel Used", "Total fuel used", "L", 0.5, 0.0, 3}},
    {0x0068, {0x0068, "Vehicle Speed", "Vehicle speed", "km/h", 1.0/256.0, 0.0, 2}},
    {0x0069, {0x0069, "Trip Distance", "Trip distance", "km", 0.125, 0.0, 3}},
    {0x006A, {0x006A, "Total Vehicle Distance", "Total vehicle distance", "km", 0.125, 0.0, 4}},
    {0x006B, {0x006B, "Battery Voltage", "Battery voltage", "V", 0.05, 0.0, 1}},
    {0x006C, {0x006C, "Alternator Voltage", "Alternator voltage", "V", 0.05, 0.0, 1}},
    {0x006D, {0x006D, "Transmission Oil Temperature", "Transmission oil temperature", "°C", 1.0, -40.0, 1}},
    {0x006E, {0x006E, "Current Gear", "Current gear", "", 1.0, 0.0, 1}},
    {0x006F, {0x006F, "Brake Application Pressure", "Brake application pressure", "kPa", 4.0, 0.0, 1}},
    {0x0070, {0x0070, "Primary Reservoir Pressure", "Primary air reservoir pressure", "kPa", 4.0, 0.0, 1}},
    {0x0071, {0x0071, "Secondary Reservoir Pressure", "Secondary air reservoir pressure", "kPa", 4.0, 0.0, 1}},
    {0x0072, {0x0072, "Active Diagnostic Codes", "Active diagnostic codes", "", 1.0, 0.0, 2}},
    {0x0073, {0x0073, "Previous Diagnostic Codes", "Previous diagnostic codes", "", 1.0, 0.0, 2}},
    {0x0074, {0x0074, "Clear Diagnostic Codes", "Clear diagnostic codes", "", 1.0, 0.0, 1}},

    // J1939 PGNs as parameters
    {0xF004, {0xF004, "EEC1 - Engine Speed", "Engine speed", "rpm", 0.125, 0.0, 2}},
    {0xFEEE, {0xFEEE, "Engine Temperature", "Engine temperature", "°C", 1.0, -40.0, 2}},
    {0xFECA, {0xFECA, "Tachograph", "Tachograph", "h", 0.05, 0.0, 3}},
    {0xFE6F, {0xFE6F, "Dash Display", "Dash display", "km/h", 0.00390625, 0.0, 2}},
    {0xF001, {0xF001, "EBC1 - Brake", "Electronic brake", "", 1.0, 0.0, 2}},
};

} // namespace ca3