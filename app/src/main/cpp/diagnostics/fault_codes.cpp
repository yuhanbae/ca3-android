#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>

namespace ca3 {

// Fault code database for Caterpillar diagnostic codes

struct FaultCode {
    uint32_t code = 0;
    std::string description;
    std::string spn;
    std::string fmi;
    std::string category;
    uint8_t severity = 0;  // 0=info, 1=warning, 2=error, 3=critical
    std::string probableCause;
    std::string correctiveAction;
};

class FaultCodeDatabase {
public:
    static const FaultCodeDatabase& instance() {
        static FaultCodeDatabase db;
        return db;
    }

    std::optional<FaultCode> getCode(uint32_t code) const {
        auto it = codes_.find(code);
        if (it != codes_.end()) return it->second;
        return std::nullopt;
    }

    std::vector<FaultCode> getCodesByCategory(const std::string& category) const {
        std::vector<FaultCode> result;
        for (const auto& pair : codes_) {
            if (pair.second.category == category) {
                result.push_back(pair.second);
            }
        }
        return result;
    }

    std::vector<FaultCode> getCodesBySPN(const std::string& spn) const {
        std::vector<FaultCode> result;
        for (const auto& pair : codes_) {
            if (pair.second.spn == spn) {
                result.push_back(pair.second);
            }
        }
        return result;
    }

    std::vector<FaultCode> getCodesByFMI(const std::string& fmi) const {
        std::vector<FaultCode> result;
        for (const auto& pair : codes_) {
            if (pair.second.fmi == fmi) {
                result.push_back(pair.second);
            }
        }
        return result;
    }

    const std::unordered_map<uint32_t, FaultCode>& allCodes() const {
        return codes_;
    }

private:
    FaultCodeDatabase() {
        initCodes();
    }

    void initCodes() {
        // Engine codes
        addCode(0x0001, "Injector Cylinder 1 Fault", "651", "7", "Engine",
                2, "Injector circuit open/short", "Check injector wiring and resistance");
        addCode(0x0002, "Injector Cylinder 2 Fault", "652", "7", "Engine",
                2, "Injector circuit open/short", "Check injector wiring and resistance");
        addCode(0x0003, "Injector Cylinder 3 Fault", "653", "7", "Engine",
                2, "Injector circuit open/short", "Check injector wiring and resistance");
        addCode(0x0004, "Injector Cylinder 4 Fault", "654", "7", "Engine",
                2, "Injector circuit open/short", "Check injector wiring and resistance");
        addCode(0x0005, "Injector Cylinder 5 Fault", "655", "7", "Engine",
                2, "Injector circuit open/short", "Check injector wiring and resistance");
        addCode(0x0006, "Injector Cylinder 6 Fault", "656", "7", "Engine",
                2, "Injector circuit open/short", "Check injector wiring and resistance");

        addCode(0x0010, "Engine Speed Sensor Fault", "190", "8", "Engine",
                3, "Crankshaft position sensor circuit", "Check sensor wiring and air gap");
        addCode(0x0011, "Camshaft Position Sensor Fault", "723", "8", "Engine",
                3, "Camshaft position sensor circuit", "Check sensor wiring and air gap");

        addCode(0x0020, "Coolant Temperature Sensor Fault", "110", "3", "Engine",
                2, "Sensor voltage high", "Check sensor and wiring");
        addCode(0x0021, "Coolant Temperature Sensor Fault", "110", "4", "Engine",
                2, "Sensor voltage low", "Check sensor and wiring");

        addCode(0x0030, "Oil Pressure Sensor Fault", "100", "3", "Engine",
                2, "Sensor voltage high", "Check sensor and wiring");
        addCode(0x0031, "Oil Pressure Sensor Fault", "100", "4", "Engine",
                2, "Sensor voltage low", "Check sensor and wiring");
        addCode(0x0032, "Oil Pressure Low", "100", "1", "Engine",
                3, "Low oil pressure", "Check oil level, pump, and sensor");

        addCode(0x0040, "Boost Pressure Sensor Fault", "102", "3", "Engine",
                2, "Sensor voltage high", "Check sensor and wiring");
        addCode(0x0041, "Boost Pressure Sensor Fault", "102", "4", "Engine",
                2, "Sensor voltage low", "Check sensor and wiring");

        addCode(0x0050, "Fuel Pressure Sensor Fault", "94", "3", "Engine",
                2, "Sensor voltage high", "Check sensor and wiring");
        addCode(0x0051, "Fuel Pressure Sensor Fault", "94", "4", "Engine",
                2, "Sensor voltage low", "Check sensor and wiring");
        addCode(0x0052, "Fuel Pressure Low", "94", "1", "Engine",
                3, "Low fuel pressure", "Check fuel filter, pump, and lines");

        addCode(0x0060, "Intake Manifold Temp Sensor Fault", "105", "3", "Engine",
                2, "Sensor voltage high", "Check sensor and wiring");
        addCode(0x0061, "Intake Manifold Temp Sensor Fault", "105", "4", "Engine",
                2, "Sensor voltage low", "Check sensor and wiring");

        addCode(0x0070, "Exhaust Gas Temperature Sensor Fault", "173", "3", "Engine",
                2, "Sensor voltage high", "Check sensor and wiring");
        addCode(0x0071, "Exhaust Gas Temperature Sensor Fault", "173", "4", "Engine",
                2, "Sensor voltage low", "Check sensor and wiring");

        // Transmission codes
        addCode(0x0100, "Transmission Input Speed Sensor Fault", "161", "8", "Transmission",
                2, "Sensor circuit", "Check sensor and wiring");
        addCode(0x0101, "Transmission Output Speed Sensor Fault", "162", "8", "Transmission",
                2, "Sensor circuit", "Check sensor and wiring");
        addCode(0x0102, "Transmission Oil Temperature Sensor Fault", "177", "3", "Transmission",
                2, "Sensor voltage high", "Check sensor and wiring");
        addCode(0x0103, "Transmission Oil Temperature Sensor Fault", "177", "4", "Transmission",
                2, "Sensor voltage low", "Check sensor and wiring");
        addCode(0x0104, "Transmission Oil Temperature High", "177", "0", "Transmission",
                3, "Transmission overheating", "Check oil level, cooler, and load");
        addCode(0x0105, "Transmission Pressure Sensor Fault", "127", "3", "Transmission",
                2, "Sensor voltage high", "Check sensor and wiring");
        addCode(0x0106, "Transmission Pressure Low", "127", "1", "Transmission",
                3, "Low transmission pressure", "Check pump, filter, and fluid level");
        addCode(0x0107, "Shift Solenoid Fault", "975", "7", "Transmission",
                2, "Solenoid circuit", "Check solenoid and wiring");
        addCode(0x0108, "Torque Converter Lockup Fault", "974", "7", "Transmission",
                2, "Lockup circuit", "Check solenoid and wiring");

        // Brake codes
        addCode(0x0200, "Brake Pressure Sensor Fault", "111", "3", "Brakes",
                2, "Sensor voltage high", "Check sensor and wiring");
        addCode(0x0201, "Brake Pressure Sensor Fault", "111", "4", "Brakes",
                2, "Sensor voltage low", "Check sensor and wiring");
        addCode(0x0202, "Primary Brake Pressure Low", "111", "1", "Brakes",
                3, "Low primary pressure", "Check compressor, leaks, and reservoir");
        addCode(0x0203, "Secondary Brake Pressure Low", "112", "1", "Brakes",
                3, "Low secondary pressure", "Check compressor, leaks, and reservoir");
        addCode(0x0204, "Park Brake Switch Fault", "560", "2", "Brakes",
                1, "Switch circuit", "Check switch and wiring");

        // Electrical codes
        addCode(0x0300, "Battery Voltage High", "168", "0", "Electrical",
                2, "Voltage above threshold", "Check alternator and regulator");
        addCode(0x0301, "Battery Voltage Low", "168", "1", "Electrical",
                2, "Voltage below threshold", "Check battery, alternator, and connections");
        addCode(0x0301, "Alternator Fault", "167", "2", "Electrical",
                2, "Alternator not charging", "Check alternator, belt, and wiring");
        addCode(0x0302, "Key Switch Circuit Fault", "158", "2", "Electrical",
                1, "Key switch circuit", "Check switch and wiring");

        // J1939 communication codes
        addCode(0x0400, "J1939 Bus Off", "0", "14", "Communication",
                2, "Bus communication error", "Check bus termination and wiring");
        addCode(0x0401, "J1939 Message Timeout", "0", "9", "Communication",
                2, "Expected message not received", "Check ECU power and bus wiring");
        addCode(0x0402, "J1939 Address Claim Failure", "0", "13", "Communication",
                2, "Address conflict", "Check ECU addresses");
        addCode(0x0403, "J1939 Data Link Fault", "0", "14", "Communication",
                2, "Data link error", "Check bus termination and wiring");

        // Aftertreatment codes
        addCode(0x0500, "Aftertreatment 1 Intake NOx Sensor Fault", "3216", "3", "Aftertreatment",
                2, "Sensor voltage high", "Check sensor and wiring");
        addCode(0x0501, "Aftertreatment 1 Intake NOx Sensor Fault", "3216", "4", "Aftertreatment",
                2, "Sensor voltage low", "Check sensor and wiring");
        addCode(0x0502, "Aftertreatment 1 Outlet NOx Sensor Fault", "3226", "3", "Aftertreatment",
                2, "Sensor voltage high", "Check sensor and wiring");
        addCode(0x0503, "Aftertreatment 1 Outlet NOx Sensor Fault", "3226", "4", "Aftertreatment",
                2, "Sensor voltage low", "Check sensor and wiring");
        addCode(0x0504, "DPF Soot Level High", "3719", "0", "Aftertreatment",
                2, "DPF needs regeneration", "Perform forced regeneration");
        addCode(0x0505, "DPF Ash Level High", "3720", "0", "Aftertreatment",
                2, "DPF needs cleaning", "Remove and clean DPF");
        addCode(0x0506, "SCR System Fault", "4360", "7", "Aftertreatment",
                2, "SCR system error", "Check DEF system and dosing valve");
        addCode(0x0507, "DEF Quality Poor", "4363", "1", "Aftertreatment",
                2, "DEF quality below spec", "Check DEF quality and replace");
        addCode(0x0508, "DEF Level Low", "1761", "1", "Aftertreatment",
                1, "DEF tank low", "Refill DEF tank");

        // Hydraulic codes
        addCode(0x0600, "Hydraulic Oil Temperature High", "177", "0", "Hydraulics",
                3, "Oil overheating", "Check oil level, cooler, and load");
        addCode(0x0601, "Hydraulic Oil Temperature Sensor Fault", "177", "3", "Hydraulics",
                2, "Sensor voltage high", "Check sensor and wiring");
        addCode(0x0602, "Hydraulic Oil Temperature Sensor Fault", "177", "4", "Hydraulics",
                2, "Sensor voltage low", "Check sensor and wiring");
        addCode(0x0603, "Hydraulic Pressure Sensor Fault", "127", "3", "Hydraulics",
                2, "Sensor voltage high", "Check sensor and wiring");
        addCode(0x0604, "Hydraulic Pressure Low", "127", "1", "Hydraulics",
                3, "Low hydraulic pressure", "Check pump, filter, and fluid level");
        addCode(0x0605, "Hydraulic Filter Restriction", "127", "14", "Hydraulics",
                2, "Filter clogged", "Replace hydraulic filter");

        // Generic/Unknown codes
        addCode(0xFFFF, "Unknown Fault Code", "0", "0", "Unknown",
                1, "Unidentified fault", "Consult service manual");
    }

    void addCode(uint32_t code, const std::string& description, const std::string& spn,
                 const std::string& fmi, const std::string& category,
                 uint8_t severity, const std::string& cause, const std::string& action) {
        FaultCode fc;
        fc.code = code;
        fc.description = description;
        fc.spn = spn;
        fc.fmi = fmi;
        fc.category = category;
        fc.severity = severity;
        fc.probableCause = cause;
        fc.correctiveAction = action;
        codes_[code] = fc;
    }

    std::unordered_map<uint32_t, FaultCode> codes_;
};

} // namespace ca3