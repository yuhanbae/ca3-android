package com.fieldtools.ca3bridge

import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbEndpoint
import android.hardware.usb.UsbInterface
import com.google.gson.annotations.SerializedName

data class UsbDeviceInfo(
    @SerializedName("deviceId") val deviceId: Int,
    @SerializedName("vendorId") val vendorId: Int,
    @SerializedName("productId") val productId: Int,
    @SerializedName("deviceClass") val deviceClass: Int,
    @SerializedName("deviceSubclass") val deviceSubclass: Int,
    @SerializedName("deviceProtocol") val deviceProtocol: Int,
    @SerializedName("manufacturerName") val manufacturerName: String?,
    @SerializedName("productName") val productName: String?,
    @SerializedName("serialNumber") val serialNumber: String?,
    @SerializedName("usbVersion") val usbVersion: Int,
    @SerializedName("interfaces") val interfaces: List<UsbInterfaceInfo>
)

data class UsbInterfaceInfo(
    @SerializedName("interfaceNumber") val interfaceNumber: Int,
    @SerializedName("interfaceClass") val interfaceClass: Int,
    @SerializedName("interfaceSubclass") val interfaceSubclass: Int,
    @SerializedName("interfaceProtocol") val interfaceProtocol: Int,
    @SerializedName("endpoints") val endpoints: List<Ca3Device.UsbEndpointInfo>
)

object UsbDescriptorParser {
    // USB endpoint direction constants
    private const val DIR_IN = 128
    private const val DIR_OUT = 0

    // USB endpoint type constants
    private const val TYPE_CONTROL = 0
    private const val TYPE_ISO = 1
    private const val TYPE_BULK = 2
    private const val TYPE_INTERRUPT = 3

    fun parse(device: UsbDevice): UsbDeviceInfo {
        val interfaces = mutableListOf<UsbInterfaceInfo>()

        for (i in 0 until device.interfaceCount) {
            val iface = device.getInterface(i)
            val endpoints = mutableListOf<Ca3Device.UsbEndpointInfo>()

            for (e in 0 until iface.endpointCount) {
                val ep = iface.getEndpoint(e)
                endpoints.add(Ca3Device.UsbEndpointInfo(
                    endpointNumber = ep.endpointNumber,
                    direction = if ((ep.direction and DIR_IN) != 0) "IN" else "OUT",
                    transferType = transferTypeToString(ep.type),
                    maxPacketSize = ep.maxPacketSize,
                    interval = ep.interval,
                    address = ep.address
                ))
            }

            interfaces.add(UsbInterfaceInfo(
                interfaceNumber = iface.id,
                interfaceClass = iface.interfaceClass,
                interfaceSubclass = iface.interfaceSubclass,
                interfaceProtocol = iface.interfaceProtocol,
                endpoints = endpoints
            ))
        }

        return UsbDeviceInfo(
            deviceId = device.deviceId,
            vendorId = device.vendorId,
            productId = device.productId,
            deviceClass = device.deviceClass,
            deviceSubclass = device.deviceSubclass,
            deviceProtocol = device.deviceProtocol,
            manufacturerName = device.manufacturerName,
            productName = device.productName,
            serialNumber = device.serialNumber,
            usbVersion = device.version,
            interfaces = interfaces
        )
    }

    private fun transferTypeToString(type: Int): String {
        return when (type) {
            0 -> "CONTROL"
            1 -> "ISOCHRONOUS"
            2 -> "BULK"
            3 -> "INTERRUPT"
            else -> "UNKNOWN"
        }
    }
}