#include "j1939.hpp"
#include <algorithm>

namespace ca3 {

// J1939 name database for human-readable output
static const std::unordered_map<uint32_t, std::string> pgn_names = {
    {0xFEF1, "Vehicle Distance"},
    {0xFEF2, "Vehicle Position"},
    {0xFEF5, "Engine Hours"},
    {0xFEF6, "Vehicle Direction/Speed"},
    {0xFEF7, "Vehicle Distance"},
    {0xFEEE, "Engine Temperature"},
    {0xFEE8, "Ambient Conditions"},
    {0xFEE4, "Fuel Economy"},
    {0xFEE0, "Engine Fluid Level/Pressure"},
    {0xFEFC, "Dash Display"},
    {0xFECA, "Tachograph"},
    {0xFE6F, "Dash Display"},
    {0xF004, "EEC1 - Electronic Engine Controller 1"},
    {0xF003, "EEC2 - Electronic Engine Controller 2"},
    {0xF002, "EEC3 - Electronic Engine Controller 3"},
    {0xF001, "EBC1 - Electronic Brake Controller 1"},
    {0xF000, "EBC2 - Electronic Brake Controller 2"},
    {0x00FE, "AT1TI - Aftertreatment 1 Intake"},
    {0x00FC, "AT1TO - Aftertreatment 1 Outlet"},
    {0x00FD, "AT2TI - Aftertreatment 2 Intake"},
    {0x00FB, "AT2TO - Aftertreatment 2 Outlet"},
    {0xEC00, "TP.CM - Transport Protocol Connection Management"},
    {0xEB00, "TP.DT - Transport Protocol Data Transfer"},
    {0xDA00, "Request"},
    {0xD900, "Acknowledge"},
    {0xEA00, "Proprietary A"},
    {0xEBFF, "Proprietary B"}
};

static const std::unordered_map<uint32_t, std::string> spn_names = {
    {190, "Engine Speed"},
    {91, "Accelerator Pedal Position 1"},
    {92, "Engine Percent Load At Current Speed"},
    {94, "Engine Fuel Delivery Pressure"},
    {97, "Water In Fuel Indicator"},
    {98, "Engine Oil Level"},
    {100, "Engine Oil Pressure"},
    {101, "Engine Crankcase Pressure"},
    {102, "Engine Intake Manifold Pressure"},
    {103, "Engine Turbocharger Speed"},
    {105, "Engine Intake Manifold Temperature"},
    {106, "Engine Intake Air Pressure"},
    {107, "Engine Air Filter 1 Differential Pressure"},
    {108, "Engine Air Filter 2 Differential Pressure"},
    {109, "Engine Coolant Pressure"},
    {110, "Engine Coolant Temperature"},
    {111, "Engine Coolant Level"},
    {157, "Injector Metering Rail 1 Pressure"},
    {158, "Key Switch Battery Potential"},
    {164, "Injection Control Pressure"},
    {166, "Engine Rated Power"},
    {167, "Alternator Potential (Voltage)"},
    {168, "Battery Potential / Power Input 1"},
    {171, "Ambient Air Temperature"},
    {172, "Engine Air Intake Temperature"},
    {173, "Engine Exhaust Gas Temperature"},
    {174, "Engine Fuel Temperature 1"},
    {175, "Engine Oil Temperature 1"},
    {176, "Turbocharger Oil Temperature"},
    {177, "Transmission Oil Temperature"},
    {182, "Engine Trip Fuel"},
    {183, "Engine Fuel Rate"},
    {184, "Engine Instantaneous Fuel Economy"},
    {185, "Engine Average Fuel Economy"},
    {186, "Engine Trip Fuel (Gaseous)"},
    {187, "Engine Fuel Rate (Gaseous)"},
    {188, "Engine Idle Shutdown Timer State"},
    {189, "Engine Rated Speed"},
    {190, "Engine Speed"},
    {235, "Engine Total Idle Hours"},
    {236, "Engine Total Revolutions"},
    {245, "Trip Distance"},
    {247, "Total Engine Hours"},
    {250, "Total Fuel Used"},
    {512, "Driver's Demand Engine - Percent Torque"},
    {513, "Actual Engine - Percent Torque"},
    {514, "Nominal Friction - Percent Torque"},
    {515, "Engine's Desired Operating Speed"},
    {516, "Engine Intake Manifold 1 Temperature"},
    {517, "Engine Exhaust Gas Recirculation 1 Mass Flow Rate"},
    {518, "Engine Exhaust Gas Recirculation 1 Temperature"},
    {519, "Engine Exhaust Gas Recirculation 1 Cooler Temperature"},
    {519, "Engine Exhaust Gas Recirculation 1 Cooler Efficiency"},
    {520, "Engine Intake Manifold 2 Temperature"},
    {521, "Engine Exhaust Gas Recirculation 2 Mass Flow Rate"},
    {522, "Engine Exhaust Gas Recirculation 2 Temperature"},
    {523, "Engine Exhaust Gas Recirculation 2 Cooler Temperature"},
    {524, "Engine Exhaust Gas Recirculation 2 Cooler Efficiency"},
    {525, "Engine Turbocharger 1 Compressor Intake Temperature"},
    {526, "Engine Turbocharger 1 Compressor Outlet Temperature"},
    {527, "Engine Turbocharger 1 Compressor Inlet Pressure"},
    {528, "Engine Turbocharger 1 Compressor Outlet Pressure"},
    {529, "Engine Turbocharger 1 Turbine Intake Temperature"},
    {530, "Engine Turbocharger 1 Turbine Outlet Temperature"},
    {531, "Engine Turbocharger 1 Turbine Inlet Pressure"},
    {532, "Engine Turbocharger 1 Turbine Outlet Pressure"},
    {533, "Engine Turbocharger 1 Speed"},
    {534, "Engine Turbocharger 2 Compressor Intake Temperature"},
    {535, "Engine Turbocharger 2 Compressor Outlet Temperature"},
    {536, "Engine Turbocharger 2 Compressor Inlet Pressure"},
    {537, "Engine Turbocharger 2 Compressor Outlet Pressure"},
    {538, "Engine Turbocharger 2 Turbine Intake Temperature"},
    {539, "Engine Turbocharger 2 Turbine Outlet Temperature"},
    {540, "Engine Turbocharger 2 Turbine Inlet Pressure"},
    {541, "Engine Turbocharger 2 Turbine Outlet Pressure"},
    {542, "Engine Turbocharger 2 Speed"},
    {543, "Engine Turbocharger 3 Compressor Intake Temperature"},
    {544, "Engine Turbocharger 3 Compressor Outlet Temperature"},
    {545, "Engine Turbocharger 3 Compressor Inlet Pressure"},
    {546, "Engine Turbocharger 3 Compressor Outlet Pressure"},
    {547, "Engine Turbocharger 3 Turbine Intake Temperature"},
    {548, "Engine Turbocharger 3 Turbine Outlet Temperature"},
    {549, "Engine Turbocharger 3 Turbine Inlet Pressure"},
    {550, "Engine Turbocharger 3 Turbine Outlet Pressure"},
    {551, "Engine Turbocharger 3 Speed"},
    {552, "Engine Turbocharger 4 Compressor Intake Temperature"},
    {553, "Engine Turbocharger 4 Compressor Outlet Temperature"},
    {554, "Engine Turbocharger 4 Compressor Inlet Pressure"},
    {555, "Engine Turbocharger 4 Compressor Outlet Pressure"},
    {556, "Engine Turbocharger 4 Turbine Intake Temperature"},
    {557, "Engine Turbocharger 4 Turbine Outlet Temperature"},
    {558, "Engine Turbocharger 4 Turbine Inlet Pressure"},
    {559, "Engine Turbocharger 4 Turbine Outlet Pressure"},
    {560, "Engine Turbocharger 4 Speed"},
    {561, "ASR Engine Control"},
    {562, "ASR Brake Control"},
    {563, "Anti-Lock Braking (ABS) Active"},
    {564, "ASR Engine Control Active"},
    {565, "ASR Brake Control Active"},
    {566, "ASR Engine Control Request"},
    {566, "ASR Brake Control Request"},
    {573, "Brake System Hold Signal"},
    {574, "Brake System Hold Signal"},
    {575, "ABS/EBS Amber Warning Signal"},
    {576, "ABS/EBS Red Warning Signal"},
    {577, "Traction Control Active"},
    {578, "Stability Control Active"},
    {579, "Stability Control Warning Signal"},
    {580, "Engine Retarder Selection"},
    {581, "Engine Retarder Status"},
    {582, "Engine Retarder Enable Switch"},
    {583, "Engine Retarder Torque Mode"},
    {584, "Engine Retarder Percent Torque"},
    {585, "Engine Retarder Speed"},
    {586, "Engine Retarder Gear"},
    {587, "Engine Retarder Inhibit Request"},
    {588, "Engine Retarder Mode"},
    {589, "Engine Retarder Selection Non-Engine"},
    {590, "Engine Retarder Enable Switch Non-Engine"},
    {591, "Engine Retarder Torque Mode Non-Engine"},
    {592, "Engine Retarder Percent Torque Non-Engine"},
    {593, "Engine Retarder Speed Non-Engine"},
    {594, "Engine Retarder Gear Non-Engine"},
    {595, "Engine Retarder Inhibit Request Non-Engine"},
    {596, "Engine Retarder Mode Non-Engine"},
    {917, "Total Vehicle Distance"},
    {918, "Trip Distance"},
    {919, "Total Fuel Used (Gaseous)"},
    {920, "Trip Fuel (Gaseous)"},
    {921, "Engine Total Hours of Operation"},
    {922, "Engine Trip Hours"},
    {923, "Engine Total Revolutions"},
    {924, "Engine Trip Revolutions"},
    {925, "Engine Total Fuel Used"},
    {926, "Engine Trip Fuel"},
    {927, "Engine Total Idle Fuel Used"},
    {928, "Engine Trip Idle Fuel Used"},
    {929, "Engine Total Idle Hours"},
    {930, "Engine Trip Idle Hours"},
    {931, "Engine Total PTO Hours"},
    {932, "Engine Trip PTO Hours"},
    {933, "Engine Total PTO Revolutions"},
    {934, "Engine Trip PTO Revolutions"},
    {935, "Engine Total PTO Fuel Used"},
    {936, "Engine Trip PTO Fuel Used"},
    {937, "Engine Total PTO Idle Hours"},
    {938, "Engine Trip PTO Idle Hours"},
    {2979, "Vehicle Speed"},
    {2980, "Wheel-Based Vehicle Speed"},
    {2981, "Vehicle Odometer"},
    {2982, "Vehicle Trip Odometer"},
    {2983, "Vehicle Direction"},
    {2984, "Vehicle Lateral Acceleration"},
    {2985, "Vehicle Longitudinal Acceleration"},
    {2986, "Vehicle Vertical Acceleration"},
    {2987, "Vehicle Yaw Rate"},
    {2988, "Vehicle Roll Angle"},
    {2989, "Vehicle Pitch Angle"}
};

std::string getPGNName(uint32_t pgn) {
    auto it = pgn_names.find(pgn);
    if (it != pgn_names.end()) return it->second;
    return "Unknown PGN (0x" + std::to_string(pgn) + ")";
}

std::string getSPNName(uint32_t spn) {
    auto it = spn_names.find(spn);
    if (it != spn_names.end()) return it->second;
    return "Unknown SPN (" + std::to_string(spn) + ")";
}

// J1939 Frame formatting for logging
std::string formatJ1939Frame(const J1939Frame& frame) {
    char buf[256];
    uint32_t pgn = frame.getPGN();
    uint8_t sa = frame.getSourceAddress();
    uint8_t da = frame.getDestinationAddress();
    uint8_t pri = frame.getPriority();
    
    snprintf(buf, sizeof(buf),
        "J1939: PGN=0x%05X(%s) SA=%d DA=%d Pri=%d DLC=%d Data=",
        pgn, getPGNName(pgn).c_str(), sa, da, pri, frame.dlc);
    
    std::string result = buf;
    for (int i = 0; i < frame.dlc; ++i) {
        char hex[4];
        snprintf(hex, sizeof(hex), "%02X ", frame.data[i]);
        result += hex;
    }
    
    // Add timestamp
    char timebuf[32];
    snprintf(timebuf, sizeof(timebuf), " @%lluns", frame.timestamp_ns);
    result += timebuf;
    
    return result;
}

// Decode J1939 frame using database
std::vector<std::pair<std::string, double>> decodeJ1939Frame(const J1939Frame& frame) {
    auto pgn_opt = J1939Database::instance().getPGN(frame.getPGN());
    if (!pgn_opt) return {};
    return pgn_opt->decode(frame.data);
}

} // namespace ca3