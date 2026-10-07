cmake_minimum_required(VERSION 3.22)

project(ca3native LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(
    ca3native
    SHARED

    ca3_usb.cpp
    ca3_crc.cpp
    ca3_frame.cpp
    ca3_jni.cpp
    ca3_protocol.cpp
    ca3/ca3_transport.cpp
    can/can_bus.cpp
    cat/cat_transport.cpp
    diagnostics/ecu.cpp
    diagnostics/fault_codes.cpp
    j1939/j1939.cpp
    j1939/pgn.cpp
    j1939/transport.cpp
    plus1/plus1_transport.cpp
    capture/capture.cpp
    usb/usb_transport.cpp
    jni/native_bridge.cpp
)

target_include_directories(
    ca3native
    PRIVATE

    include
    ca3
    can
    cat
    diagnostics
    j1939
    plus1
    capture
    usb
)

find_library(
    log-lib
    log
)

target_link_libraries(
    ca3native
    ${log-lib}
)

target_compile_options(
    ca3native
    PRIVATE

    -Wall
    -Wextra
    -Wpedantic
    -fexceptions
    -frtti
)