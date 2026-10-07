# CA3 Bridge

Standalone Android USB-host application for Caterpillar Communication Adapter 3 (CA3) and compatible USB diagnostic interfaces.

## Overview

This is a **standalone Android application** that communicates with CA3 and compatible USB diagnostic adapters using Android's native USB Host APIs. No Termux, no Python, no external daemons, no root required.

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

- Android SDK (API 35)
- Android NDK (r28+)
- Gradle 9.8+
- Kotlin 2.0.0
- CMake 3.31.6

### Build on Linux/macOS (canonical)

```bash
cd ca3-android
./gradlew assembleRelease
```

### Build on CI/CD (canonical)

**GitHub Actions** is the canonical build environment:

```yaml
# .github/workflows/android.yml
# ubuntu-24.04, JDK 21, Android SDK 35, NDK r28, CMake 3.31.6
```

### Build on Termux (NOT supported)

Termux is **NOT a supported build environment** due to fundamental libc incompatibility:
- Termux uses bionic libc
- Gradle native platform and aapt2 require glibc (libstdc++.so.6)
- Native library compilation and aapt2 R class generation will fail

### Output

```
app/build/outputs/apk/release/app-release.apk
lib/arm64-v8a/libca3native.so
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
8. **Run USB Smoke Test** with explicit probe bytes (no guessing)
9. **Capture raw USB traffic** for protocol analysis

## Project Structure

```
ca3-android/
├── .github/workflows/android.yml    # GitHub Actions CI/CD
├── app/
│   ├── src/main/
│   │   ├── AndroidManifest.xml
│   │   ├── java/com/fieldtools/ca3bridge/
│   │   │   ├── MainActivity.kt           # Main UI
│   │   │   ├── Ca3Device.kt              # USB connection logic
│   │   │   ├── Ca3UsbSmokeTest.kt        # USB RX/TX smoke test
│   │   │   ├── ProtocolLogger.kt         # Capture engine + SQLite
│   │   │   ├── UsbService.kt             # Background USB monitor
│   │   │   ├── UsbDeviceInfo.kt          # USB descriptor parser
│   │   │   ├── NativeBridge.kt           # JNI wrapper
│   │   │   ├── UsbDeviceInfo.kt          # USB descriptor parser
│   │   │   ├── DeviceAdapter.kt          # USB device RecyclerView adapter
│   │   │   └── Ca3Application.kt         # Application class
│   │   ├── cpp/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── include/ca3_native.h      # C ABI header
│   │   │   ├── ca3_usb.cpp               # Native USB transport
│   │   │   ├── jni/native_bridge.cpp     # JNI entry point
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

`libca3native.so` (ARM64) provides:

- **C ABI** (not C++ classes directly to Kotlin):
  - `ca3_usb_create/destroy`
  - `ca3_usb_set_connection/interface/endpoints`
  - `ca3_usb_write/read/get_info/last_error`
- **Thread-safe** opaque handle pattern
- **JNI bridge** (`native_bridge.cpp`) delegates USB I/O to Android's `UsbDeviceConnection.bulkTransfer()`

## Protocol Support

| Protocol | Status | Notes |
|----------|--------|-------|
| CA3 Transport (USB) | ✅ USB transport implemented; hardware validation pending | USB bulk/interrupt/control |
| CAT (Caterpillar) | 🟡 Software module; hardware validation pending | Security access, DTCs, live data, programming |
| J1939 | 🟡 Software module; hardware validation pending | PGN/SPN database, Transport Protocol (BAM/RTS/CTS) |
| CAN | 🟡 Software module; hardware validation pending | Frame parsing, bit timing |
| PLUS+1 (Danfoss) | 🟡 Software module; hardware validation pending | SDO read/write, identity |
| ISO-TP | 🔄 Planned | ISO 15765-2 transport |
| KWP2000 | 🔄 Planned | Keyword Protocol 2000 |
| UDS | 🔄 Planned | Unified Diagnostic Services |

**Legend**: ✅ = Hardware-validated, 🟡 = Software module implemented (awaiting hardware), 🔄 = Planned

## USB RX/TX Smoke Test

The app includes a **Ca3UsbSmokeTest** diagnostic that performs:

1. **USB enumeration** → VID/PID/manufacturer/product
2. **Permission grant** → Android USB permission dialog
3. **Interface claim** → `claimInterface()`
4. **Endpoint discovery** → IN/OUT bulk endpoints
5. **JNI attach** → Native bridge attaches `UsbDeviceConnection`
6. **TX** → Configurable probe bytes (explicit, no guessing)
7. **RX** → Read from IN endpoint, logs hex
8. **Raw capture** → Binary/JSONL/CSV export

**Critical**: The probe bytes are **explicitly supplied by the user**, not guessed. Generic patterns like `00 00 00 00` or `55 AA` do not prove CA3 communication.

## Hardware Validation Checklist

Before claiming protocol support, each layer must be validated on real hardware:

### Layer 1: USB Transport (TEST-01 through TEST-04)
- [ ] TEST-01: USB device visible & Android permission granted
- [ ] TEST-02: `UsbDeviceConnection` opened, correct interface claimed
- [ ] TEST-03: Endpoints discovered (IN/OUT bulk)
- [ ] TEST-02: `libca3native.so` loads, `nativeCreate()` succeeds
- [ ] TEST-02: `nativeAttachConnection()` succeeds
- [ ] TEST-03: `bulkTransfer(OUT)` returns N > 0
- [ ] TEST-04: `bulkTransfer(IN)` returns N ≥ 0

### Layer 2: CA3 Framing (TEST-05)
- [ ] CA3-specific request sent
- [ ] CA3-specific response received
- [ ] Response parser validates frame structure

### Layer 3: CAN/J1939 (TEST-06)
- [ ] CAN frames extracted from CA3 frames
- [ ] J1939 PGN/SPN decoding verified

### Layer 4: CAT/PLUS+1 (TEST-07+)
- [ ] CAT security access handshake
- [ ] CAT DTC read / live data stream
- [ ] PLUS+1 SDO read/write verified

**Only after all layers validated** should protocol support be marked as "Implemented".

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

# Connected device tests (requires hardware)
./gradlew connectedAndroidTest
```

## Known Limitations

- **Hardware validation pending** - All protocol claims are software-only until validated on real CA3
- **CA3 protocol reverse-engineered** - Not official specification
- **Programming/flashing disabled** - Safety protection enabled
- **AI assistant not integrated** - Optional future feature

## CI/CD

**Canonical build**: GitHub Actions (`.github/workflows/android.yml`)
- Runner: `ubuntu-24.04`
- JDK: Temurin 21
- Android SDK: 35, Build Tools 35.0.0
- NDK: r28 (28.2.13676358)
- CMake: 3.31.6

### Artifacts
- `app-release.apk` (signed, release)
- `libca3native.so` (arm64-v8a)

## License

Proprietary - For authorized diagnostic use only.

## Disclaimer

This tool is for educational and authorized diagnostic purposes only. Unauthorized access to vehicle systems may violate laws and regulations. Always verify against official Caterpillar SIS procedures.