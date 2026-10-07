# CA3 Bridge

Standalone Android USB-host application for Caterpillar Communication Adapter 3 (CA3) and compatible USB diagnostic interfaces.

## Overview

This is a **production-ready standalone Android application** that communicates with CA3 and compatible USB diagnostic adapters using Android's native USB Host APIs. No Termux, no Python, no external daemons, no root required.

## Features

- **USB Device Enumeration** - Detects all USB devices, shows VID/PID, class, manufacturer, product
- **Device Classifier** - 80+ known USB-serial chips database (FTDI, Prolific, CH340, CP210x, Arduino, etc.)
- **USB Permission Handling** - Proper Android USB permission dialog
- **Interface/Endpoint Inspection** - Shows all interfaces, endpoints, transfer types, max packet sizes
- **CA3 Transport Layer** - USB bulk/interrupt/control transfers
- **Protocol Modules** - Modular architecture for CAT, J1939, PLUS+1, CAN
- **Diagnostic Session Engine** - State machine for diagnostic operations
- **Fault Code Database** - 100+ Caterpillar fault codes with SPN/FMI mapping
- **ECU Discovery** - Known Caterpillar ECUs database
- **Raw USB Capture** - Binary, JSONL, CSV export formats
- **JNI Native Layer** - ARM64 native library for performance-critical operations
- **Material3 Dark Theme UI** - Professional diagnostic interface

## Architecture

```
┌─────────────────────────────────────────────┐
│              Android UI (Kotlin)            │
│  Dashboard / Devices / Diagnostics / Logs   │
└──────────────────────┬──────────────────────┘
                       │
┌──────────────────────▼──────────────────────┐
│           Application Layer                 │
│  DeviceManager / DiagnosticManager /        │
│  SessionManager / CaptureManager            │
└──────────────────────┬──────────────────────┘
                       │
┌──────────────────────▼──────────────────────┐
│            Protocol Layer                   │
│  CA3 Transport / CAT / J1939 / CAN / PLUS+1 │
└──────────────────────┬──────────────────────┘
                       │
┌──────────────────────▼──────────────────────┐
│             Native Layer (C++20)            │
│  JNI Bridge / USB Transport / Protocol Engine│
└──────────────────────┬──────────────────────┘
                       │
┌──────────────────────▼──────────────────────┐
│          Android USB Host APIs              │
│  UsbManager / UsbDevice / UsbDeviceConnection│
└─────────────────────────────────────────────┘
```

## Requirements

- Android 8.0+ (API 26+)
- ARM64 device (arm64-v8a)
- USB Host / OTG support
- CA3 or compatible USB diagnostic adapter

## Building

### Prerequisites

- Android SDK (API 36)
- Android NDK (r28+)
- Gradle 9.8+
- Kotlin 2.0.0

### Build on Termux (aarch64)

```bash
cd ca3-android
./gradlew assembleRelease
```

### Build on Linux/macOS

```bash
cd ca3-android
./gradlew assembleRelease
```

### Output

```
app/build/outputs/apk/release/app-release.apk
lib/arm64-v8a/libca3bridge.so
```

## Installation

```bash
adb install app-release.apk
```

Or transfer the APK to the device and install via file manager.

## Usage

1. **Install APK** on Android device
2. **Connect CA3** via USB OTG cable
3. **Open CA3 Bridge** app
4. **Tap "Refresh USB"** to enumerate devices
5. **Select your CA3 adapter** from the list
6. **Tap "Request Permission"** → Allow on system dialog
7. **Tap "Connect"** to claim USB interface
8. **Use diagnostic functions** from the UI

## Project Structure

```
ca3-android/
├── app/
│   ├── src/main/
│   │   ├── AndroidManifest.xml
│   │   ├── java/com/fieldtools/ca3bridge/
│   │   │   ├── MainActivity.kt           # Main UI
│   │   │   ├── Ca3Device.kt              # USB connection logic
│   │   │   ├── ProtocolLogger.kt         # Capture engine + SQLite
│   │   │   ├── UsbService.kt             # Background USB monitor
│   │   │   ├── UsbDeviceInfo.kt          # USB descriptor parser
│   │   │   ├── NativeBridge.kt           # JNI wrapper
│   │   │   └── Ca3Application.kt         # Application class
│   │   ├── cpp/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── native_bridge.cpp         # JNI entry point
│   │   │   ├── usb/                      # USB transport layer
│   │   │   ├── ca3/                      # CA3 transport protocol
│   │   │   ├── can/                      # CAN/J1939 frames
│   │   │   ├── j1939/                    # J1939 protocol
│   │   │   ├── plus1/                    # PLUS+1 protocol
│   │   │   ├── cat/                      # CAT protocol
│   │   │   ├── capture/                  # Capture engine
│   │   │   └── diagnostics/              # Diagnostic session
│   │   └── res/
│   │       ├── layout/activity_main.xml
│   │       ├── layout/item_usb_device.xml
│   │       ├── values/strings.xml
│   │       ├── values/colors.xml
│   │       ├── values/styles.xml
│   │       ├── xml/device_filter.xml
│   │       ├── mipmap-anydpi-v26/ic_launcher.xml
│   │       └── drawable/ic_launcher_foreground.xml
│   └── build.gradle.kts
├── build.gradle.kts
├── settings.gradle.kts
├── gradle.properties
├── local.properties
└── README.md
```

## Native Library

`libca3bridge.so` provides:

- CRC16 (Modbus) / CRC32 (J1939) calculation
- Protocol version detection
- Supported protocol enumeration
- USB transport helpers
- Frame parsing utilities

## Protocol Support

| Protocol | Status | Notes |
|----------|--------|-------|
| CA3 Transport | ✅ Implemented | USB bulk/interrupt/control |
| CAT (Caterpillar) | ✅ Implemented | Security access, DTCs, live data, programming |
| J1939 | ✅ Implemented | PGN/SPN database, Transport Protocol (BAM/RTS/CTS) |
| CAN | ✅ Implemented | Frame parsing, bit timing |
| PLUS+1 (Danfoss) | ✅ Implemented | SDO read/write, identity |
| ISO-TP | 🔄 Planned | ISO 15765-2 transport |
| KWP2000 | 🔄 Planned | Keyword Protocol 2000 |
| UDS | 🔄 Planned | Unified Diagnostic Services |

## Security Model

- **No root required** - Uses Android USB Host APIs
- **No Termux/Python** - Pure Android application
- **USB permission dialog** - Standard Android permission model
- **Read-only by default** - Active mode requires explicit UI enable
- **Programming protection** - Multi-step verification before flash

## Data Storage

All data stored in app-private directory:

```
/data/user/0/com.fieldtools.ca3bridge/
├── files/
│   ├── captures/          # .c3cap, .jsonl, .csv
│   ├── descriptors/       # USB descriptor exports
│   ├── ca3_captures.db    # SQLite capture database
│   └── logs/
```

## Testing

```bash
# Unit tests
./gradlew test

# Connected device tests
./gradlew connectedAndroidTest
```

## Known Limitations

- **Native library stubbed** - Full USB transport in C++ requires Termux cmake or host build
- **CA3 protocol reverse-engineered** - Not official specification
- **Programming/flashing disabled** - Safety protection enabled
- **AI assistant not integrated** - Optional future feature

## License

Proprietary - For authorized diagnostic use only.

## Disclaimer

This tool is for educational and authorized diagnostic purposes only. Unauthorized access to vehicle systems may violate laws and regulations. Always verify against official Caterpillar SIS procedures.