package com.fieldtools.ca3bridge

import android.hardware.usb.UsbDeviceConnection
import android.hardware.usb.UsbEndpoint

class NativeBridge {

    companion object {
        init {
            System.loadLibrary("ca3native")
        }
    }

    private var handle: Long = 0L

    fun create() {
        check(handle == 0L) {
            "Native bridge already created"
        }

        handle = nativeCreate()

        check(handle != 0L) {
            "nativeCreate() failed"
        }
    }

    fun destroy() {
        if (handle != 0L) {
            nativeDestroy(handle)
            handle = 0L
        }
    }

    fun attach(
        connection: UsbDeviceConnection
    ): Boolean {
        check(handle != 0L) {
            "Native bridge not created"
        }

        return nativeAttachConnection(
            handle,
            connection
        )
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

        return nativeBulkTransfer(
            handle,
            endpoint,
            data,
            offset,
            length,
            timeoutMs
        )
    }

    fun setInterface(interfaceNumber: Int): Boolean {
        check(handle != 0L) { "Native bridge not created" }
        return nativeSetInterface(handle, interfaceNumber) == 0
    }

    fun setEndpoints(epIn: Int, epOut: Int): Boolean {
        check(handle != 0L) { "Native bridge not created" }
        return nativeSetEndpoints(handle, epIn, epOut) == 0
    }

    fun getInfo(): ca3_usb_info_t? {
        check(handle != 0L) { "Native bridge not created" }
        val info = ca3_usb_info_t()
        val result = nativeGetInfo(handle, info)
        return if (result == 0) info else null
    }

    fun lastError(): String? {
        if (handle == 0L) return "invalid handle"
        val error = nativeLastError(handle)
        return if (error != null && error.isNotEmpty()) error else null
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