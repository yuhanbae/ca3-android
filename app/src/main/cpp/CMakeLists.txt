cmake_minimum_required(VERSION 3.22)

project(ca3native LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(
    ca3native
    SHARED

    app/src/main/cpp/ca3_usb.cpp
    app/src/main/cpp/jni/native_bridge.cpp
)

target_include_directories(
    ca3native
    PRIVATE

    app/src/main/cpp/include
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