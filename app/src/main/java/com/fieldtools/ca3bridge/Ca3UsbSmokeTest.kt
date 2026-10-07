package com.fieldtools.ca3bridge

import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbDeviceConnection
import android.hardware.usb.UsbEndpoint
import android.util.Log

class Ca3UsbSmokeTest {

    companion object {
        private const val TAG = "CA3Smoke"
    }

    data class Result(
        val success: Boolean,
        val message: String,
        val txBytes: Int = 0,
        val rxBytes: Int = 0,
        val rxHex: String = ""
    )

    fun run(
        device: UsbDevice,
        connection: UsbDeviceConnection,
        usbInterface: android.hardware.usb.UsbInterface,
        epIn: UsbEndpoint,
        epOut: UsbEndpoint,
        probe: ByteArray
    ): Result {

        Log.i(
            TAG,
            "device=${device.deviceName}"
        )

        Log.i(
            TAG,
            "VID=${device.vendorId.toString(16)} " +
                "PID=${device.productId.toString(16)}"
        )

        Log.i(
            TAG,
            "interface=${usbInterface.id}"
        )

        Log.i(
            TAG,
            "IN=0x${epIn.address.toString(16)} " +
                "OUT=0x${epOut.address.toString(16)}"
        )

        if (!connection.claimInterface(
                usbInterface,
                true
            )
        ) {
            return Result(
                false,
                "claimInterface() failed"
            )
        }

        try {

            val bridge = NativeBridge()

            bridge.create()

            try {
                if (!bridge.attach(connection)) {
                    return Result(
                        false,
                        "JNI attach failed"
                    )
                }

                val usbInterfaceObj = usbInterface

                if (!bridge.setInterface(usbInterfaceObj.id)) {
                    return Result(
                        false,
                        "nativeSetInterface failed"
                    )
                }

                if (!bridge.setEndpoints(
                        epIn.address and 0xFF,
                        epOut.address and 0xFF
                    )
                ) {
                    return Result(
                        false,
                        "nativeSetEndpoints failed"
                    )
                }

                Log.i(
                    TAG,
                    "TX ${probe.size} bytes: " +
                        probe.toHex()
                )

                val tx = bridge.bulkTransfer(
                    epOut,
                    probe,
                    timeoutMs = 1000
                )

                if (tx < 0) {
                    return Result(
                        false,
                        "TX failed: $tx"
                    )
                }

                val rxBuffer =
                    ByteArray(4096)

                val rx = bridge.bulkTransfer(
                    epIn,
                    rxBuffer,
                    timeoutMs = 500
                )

                if (rx < 0) {
                    return Result(
                        false,
                        "RX failed: $rx",
                        txBytes = tx
                    )
                }

                val hex =
                    rxBuffer
                        .copyOf(rx)
                        .toHex()

                Log.i(
                    TAG,
                    "RX $rx bytes: $hex"
                )

                return Result(
                    success = true,
                    message = "USB RX/TX completed",
                    txBytes = tx,
                    rxBytes = rx,
                    rxHex = hex
                )

            } finally {
                bridge.destroy()
            }

        } finally {
            connection.releaseInterface(
                usbInterface
            )
        }
    }

    private fun ByteArray.toHex(): String =
        joinToString(" ") {
            "%02X".format(it.toInt() and 0xff)
        }
}