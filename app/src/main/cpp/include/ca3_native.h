#ifndef CA3_NATIVE_H
#define CA3_NATIVE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t vid;
    uint16_t pid;
    uint8_t interface_number;

    uint8_t ep_in;
    uint8_t ep_out;

    uint16_t ep_in_max_packet;
    uint16_t ep_out_max_packet;

    uint8_t ep_in_type;
    uint8_t ep_out_type;
} ca3_usb_info_t;

/*
 * The Java layer passes a native Android UsbDevice/UsbDeviceConnection
 * through JNI. Native code owns only its state and does not own the
 * Java USB object.
 */

typedef struct ca3_handle ca3_handle_t;

ca3_handle_t* ca3_usb_create(void);

void ca3_usb_destroy(ca3_handle_t* handle);

int ca3_usb_set_connection(
    ca3_handle_t* handle,
    void* usb_connection
);

int ca3_usb_set_interface(
    ca3_handle_t* handle,
    int interface_number
);

int ca3_usb_set_endpoints(
    ca3_handle_t* handle,
    uint8_t ep_in,
    uint8_t ep_out
);

int ca3_usb_get_info(
    ca3_handle_t* handle,
    ca3_usb_info_t* info
);

int ca3_usb_write(
    ca3_handle_t* handle,
    const uint8_t* data,
    size_t length,
    int timeout_ms
);

int ca3_usb_read(
    ca3_handle_t* handle,
    uint8_t* data,
    size_t capacity,
    int timeout_ms
);

const char* ca3_usb_last_error(
    ca3_handle_t* handle
);

#ifdef __cplusplus
}
#endif

#endif