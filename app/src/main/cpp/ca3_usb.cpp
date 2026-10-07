#include "ca3_native.h"

#include <android/log.h>

#include <cstring>
#include <mutex>
#include <string>

#define TAG "CA3Native"

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

struct ca3_handle {
    std::mutex mutex;

    void* connection = nullptr;

    int interface_number = -1;

    uint8_t ep_in = 0;
    uint8_t ep_out = 0;

    ca3_usb_info_t info{};

    std::string last_error;
};

static void set_error(
    ca3_handle_t* h,
    const char* message
) {
    if (h != nullptr) {
        h->last_error = message ? message : "unknown error";
    }

    LOGE("%s", message ? message : "unknown error");
}

extern "C"
ca3_handle_t* ca3_usb_create(void) {
    auto* h = new ca3_handle_t();

    LOGI("ca3_usb_create");

    return h;
}

extern "C"
void ca3_usb_destroy(
    ca3_handle_t* h
) {
    if (!h)
        return;

    LOGI("ca3_usb_destroy");

    delete h;
}

extern "C"
int ca3_usb_set_connection(
    ca3_handle_t* h,
    void* connection
) {
    if (!h) {
        return -1;
    }

    std::lock_guard<std::mutex> lock(h->mutex);

    if (!connection) {
        set_error(h, "null USB connection");
        return -2;
    }

    h->connection = connection;

    LOGI("USB connection attached");

    return 0;
}

extern "C"
int ca3_usb_set_interface(
    ca3_handle_t* h,
    int interface_number
) {
    if (!h)
        return -1;

    std::lock_guard<std::mutex> lock(h->mutex);

    h->interface_number = interface_number;

    LOGI(
        "interface=%d",
        interface_number
    );

    return 0;
}

extern "C"
int ca3_usb_set_endpoints(
    ca3_handle_t* h,
    uint8_t ep_in,
    uint8_t ep_out
) {
    if (!h)
        return -1;

    std::lock_guard<std::mutex> lock(h->mutex);

    h->ep_in = ep_in;
    h->ep_out = ep_out;

    LOGI(
        "endpoints IN=0x%02x OUT=0x%02x",
        ep_in,
        ep_out
    );

    return 0;
}

extern "C"
int ca3_usb_get_info(
    ca3_handle_t* h,
    ca3_usb_info_t* info
) {
    if (!h || !info)
        return -1;

    std::lock_guard<std::mutex> lock(h->mutex);

    *info = h->info;

    return 0;
}

/*
 * Actual USB I/O is deliberately routed through the Java
 * UsbDeviceConnection layer for this first implementation.
 *
 * JNI calls these functions only after the Java USB layer has
 * successfully claimed the interface and discovered endpoints.
 */

extern "C"
int ca3_usb_write(
    ca3_handle_t* h,
    const uint8_t* data,
    size_t length,
    int timeout_ms
) {
    if (!h || !data || length == 0)
        return -1;

    if (!h->connection) {
        set_error(h, "USB connection not attached");
        return -2;
    }

    /*
     * The JNI implementation performs UsbDeviceConnection.bulkTransfer().
     *
     * This function is therefore currently the native transport boundary,
     * not an unsafe direct dereference of Android framework objects.
     */

    LOGI(
        "TX request length=%zu timeout=%d",
        length,
        timeout_ms
    );

    return -100;
}

extern "C"
int ca3_usb_read(
    ca3_handle_t* h,
    uint8_t* data,
    size_t capacity,
    int timeout_ms
) {
    if (!h || !data || capacity == 0)
        return -1;

    if (!h->connection) {
        set_error(h, "USB connection not attached");
        return -2;
    }

    LOGI(
        "RX request capacity=%zu timeout=%d",
        capacity,
        timeout_ms
    );

    return -100;
}

extern "C"
const char* ca3_usb_last_error(
    ca3_handle_t* h
) {
    if (!h)
        return "invalid handle";

    return h->last_error.c_str();
}