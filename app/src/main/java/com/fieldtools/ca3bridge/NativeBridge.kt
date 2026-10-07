package com.fieldtools.ca3bridge

import android.hardware.usb.UsbDeviceConnection
import android.hardware.usb.UsbEndpoint

class NativeBridge {

    companion object {
        @Volatile
        var nativeAvailable = false

        init {
            try {
                System.loadLibrary("ca3native")
                nativeAvailable = true
            } catch (_: UnsatisfiedLinkError) {
                // Native library not available on this device/build; continue with stubs
                nativeAvailable = false
            }
        }
    }

    private var handle: Long = 0L

    fun create() {
        check(handle == 0L) {
            "Native bridge already created"
        }
        if (!companionObject.nativeAvailable) {
            handle = 1L
            return
        }
        handle = nativeCreate()
        check(handle != 0L) {
            "nativeCreate() failed"
        }
    }

    fun destroy() {
        if (handle != 0L) {
            if (companionObject.nativeAvailable) {
                nativeDestroy(handle)
            }
            handle = 0L
        }
    }

    fun attach(
        connection: UsbDeviceConnection
    ): Boolean {
        check(handle != 0L) {
            "Native bridge not created"
        }
        return if (companionObject.nativeAvailable) {
            nativeAttachConnection(handle, connection)
        } else {
            false
        }
    }

    fun bulkTransfer(
        endpoint: UsbEndpoint,
        data: ByteArray,
        offset: Int = 0,
        length: Int = data.size,
        timeoutMs: Int = 1000
    ): Int {
        check(handle != 0L) {
            "Native bridge not created"
        }
        return if (companionObject.nativeAvailable) {
            nativeBulkTransfer(handle, endpoint, data, offset, length, timeoutMs)
        } else {
            -1
        }
    }

    fun setInterface(interfaceNumber: Int): Boolean {
        check(handle != 0L) { "Native bridge not created" }
        return if (companionObject.nativeAvailable) {
            nativeSetInterface(handle, interfaceNumber) == 0
        } else {
            false
        }
    }

    fun setEndpoints(epIn: Int, epOut: Int): Boolean {
        check(handle != 0L) { "Native bridge not created" }
        return if (companionObject.nativeAvailable) {
            nativeSetEndpoints(handle, epIn, epOut) == 0
        } else {
            false
        }
    }

    fun getInfo(): ca3_usb_info_t? {
        check(handle != 0L) { "Native bridge not created" }
        return if (companionObject.nativeAvailable) {
            val info = ca3_usb_info_t()
            val result = nativeGetInfo(handle, info)
            if (result == 0) info else null
        } else {
            null
        }
    }

    fun lastError(): String? {
        if (handle == 0L) return "invalid handle"
        return if (companionObject.nativeAvailable) {
            val error = nativeLastError(handle)
            if (error != null && error.isNotEmpty()) error else null
        } else {
            null
        }
    }

    private external fun nativeCreate(): Long

    private external fun nativeDestroy(
        handle: Long
    )

    private external fun nativeAttachConnection(
        handle: Long,
        connection: UsbDeviceConnection
    ): Boolean

    private external fun nativeBulkTransfer(
        handle: Long,
        endpoint: UsbEndpoint,
        data: ByteArray,
        offset: Int,
        length: Int,
        timeoutMs: Int
    ): Int

    private external fun nativeSetInterface(
        handle: Long,
        interfaceNumber: Int
    ): Int

    private external fun nativeSetEndpoints(
        handle: Long,
        epIn: Int,
        epOut: Int
    ): Int

    private external fun nativeGetInfo(
        handle: Long,
        info: ca3_usb_info_t
    ): Int

    private external fun nativeLastError(
        handle: Long
    ): String?
}