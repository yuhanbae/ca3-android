#include "can_frame.hpp"
#include <android/log.h>

#define LOG_TAG "CANBus"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace ca3 {

CANBus::~CANBus() = default;

CA3CANBus::~CA3CANBus() = default;

bool CA3CANBus::open() {
    LOGI("CA3 CAN bus opened");
    return true;
}

void CA3CANBus::close() {
    LOGI("CA3 CAN bus closed");
}

bool CA3CANBus::send(const CANFrame& frame) {
    // Placeholder for actual CAN frame transmission via CA3
    return false;
}

void CA3CANBus::setFrameCallback(FrameCallback callback) {
    frameCallback_ = std::move(callback);
}

void CA3CANBus::setErrorCallback(ErrorCallback callback) {
    errorCallback_ = std::move(callback);
}

bool CA3CANBus::isOpen() const {
    return false;
}

} // namespace ca3