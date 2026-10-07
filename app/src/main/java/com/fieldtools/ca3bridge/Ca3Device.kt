package com.fieldtools.ca3bridge

import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbDeviceConnection
import android.hardware.usb.UsbEndpoint
import android.hardware.usb.UsbInterface
import android.hardware.usb.UsbManager
import android.util.Log
import kotlinx.coroutines.*
import java.util.Locale

data class Ca3UsbLayout(
    val interfaceNumber: Int,
    val bulkInEndpoint: UsbEndpointInfo?,
    val bulkOutEndpoint: UsbEndpointInfo?,
    val interruptInEndpoint: UsbEndpointInfo?,
    val interruptOutEndpoint: UsbEndpointInfo?
) {
    fun hasBulkEndpoints(): Boolean = bulkInEndpoint != null && bulkOutEndpoint != null
}

data class UsbEndpointInfo(
    val endpointNumber: Int,
    val direction: String,
    val transferType: String,
    val maxPacketSize: Int,
    val interval: Int,
    val address: Int
)

data class Ca3DeviceState(
    val isConnected: Boolean = false,
    val hasPermission: Boolean = false,
    val vendorId: Int = 0,
    val productId: Int = 0,
    val productName: String = "",
    val interfaceNumber: Int = -1,
    val state: String = "DISCONNECTED",
    val captureActive: Boolean = false,
    val activeMode: Boolean = false
)

enum class Ca3ConnectionState {
    DISCONNECTED,
    USB_CONNECTED,
    DESCRIPTOR_READY,
    USB_CLAIMED,
    CA3_BOOT_WAIT,
    CA3_INITIALIZING,
    CA3_READY,
    CHANNEL_OPEN,
    ACTIVE,
    ERROR
}

enum class Ca3Error {
    NONE,
    USB_ERROR,
    USB_PERMISSION_DENIED,
    USB_DISCONNECTED,
    TIMEOUT,
    MALFORMED_FRAME,
    PROTOCOL_ERROR,
    DEVICE_LOST,
    TRANSMIT_DISABLED
}

class Ca3Device(
    private val usbManager: UsbManager,
    private val device: UsbDevice,
    private val onStateChange: (Ca3ConnectionState) -> Unit,
    private val onError: (Ca3Error, String) -> Unit,
    private val onLog: (String, String, String) -> Unit,
    private val onTraffic: (TrafficRecord) -> Unit
) {
    private var connection: UsbDeviceConnection? = null
    private var claimedInterface: UsbInterface? = null
    private var layout: Ca3UsbLayout? = null
    private var currentState = Ca3ConnectionState.DISCONNECTED
    private var captureActive = false
    private var activeMode = false
    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private val usbMutex = Object()

    data class TrafficRecord(
        val timestampNs: Long,
        val direction: String,
        val transferType: String,
        val endpoint: String,
        val requestType: Int = 0,
        val request: Int = 0,
        val value: Int = 0,
        val index: Int = 0,
        val payload: ByteArray
    )

    fun getState(): Ca3DeviceState = Ca3DeviceState(
        isConnected = connection != null,
        hasPermission = usbManager.hasPermission(device),
        vendorId = device.vendorId,
        productId = device.productId,
        productName = device.productName ?: "",
        interfaceNumber = layout?.interfaceNumber ?: -1,
        state = currentState.name,
        captureActive = captureActive,
        activeMode = activeMode
    )

    fun requestPermission(permissionIntent: android.app.PendingIntent) {
        log("USB", "INFO", "Requesting USB permission for device ${device.deviceId}")
        usbManager.requestPermission(device, permissionIntent)
    }

    fun onPermissionResult(granted: Boolean) {
        if (granted) {
            log("USB", "INFO", "USB permission granted")
            onStateChange(Ca3ConnectionState.USB_CONNECTED)
            scope.launch { connectUsb() }
        } else {
            log("USB", "ERROR", "USB permission denied")
            onError(Ca3Error.USB_PERMISSION_DENIED, "User denied USB permission")
            onStateChange(Ca3ConnectionState.ERROR)
        }
    }

    private suspend fun connectUsb() {
        synchronized(usbMutex) {
            try {
                log("USB", "INFO", "Opening device connection")
                val conn = usbManager.openDevice(device)
                if (conn == null) {
                    onError(Ca3Error.USB_ERROR, "Failed to open device connection")
                    onStateChange(Ca3ConnectionState.ERROR)
                    return@synchronized
                }
                connection = conn
                onStateChange(Ca3ConnectionState.DESCRIPTOR_READY)

                val selectedLayout = selectInterface()
                if (selectedLayout == null) {
                    onError(Ca3Error.USB_ERROR, "No suitable interface found")
                    onStateChange(Ca3ConnectionState.ERROR)
                    return@synchronized
                }

                layout = selectedLayout
                log("USB", "INFO", "Selected interface ${layout!!.interfaceNumber} with bulk endpoints")

                val iface = device.getInterface(layout!!.interfaceNumber)
                if (!conn.claimInterface(iface, true)) {
                    onError(Ca3Error.USB_ERROR, "Failed to claim interface")
                    onStateChange(Ca3ConnectionState.ERROR)
                    return@synchronized
                }
                claimedInterface = iface
                onStateChange(Ca3ConnectionState.USB_CLAIMED)
                log("USB", "INFO", "Interface claimed successfully")

                onStateChange(Ca3ConnectionState.CA3_BOOT_WAIT)
                delay(500)

                onStateChange(Ca3ConnectionState.CA3_INITIALIZING)
                initializeCa3()

                onStateChange(Ca3ConnectionState.CA3_READY)
                log("USB", "INFO", "CA3 device ready for communication")

                startReadLoop()

            } catch (e: Exception) {
                log("USB", "ERROR", "Connection failed: ${e.message}")
                onError(Ca3Error.USB_ERROR, e.message ?: "Unknown error")
                onStateChange(Ca3ConnectionState.ERROR)
            }
        }
    }

    private fun selectInterface(): Ca3UsbLayout? {
        var bestLayout: Ca3UsbLayout? = null
        var bestScore = 0

        for (i in 0 until device.interfaceCount) {
            val iface = device.getInterface(i)
            var bulkIn: UsbEndpointInfo? = null
            var bulkOut: UsbEndpointInfo? = null
            var interruptIn: UsbEndpointInfo? = null
            var interruptOut: UsbEndpointInfo? = null

            for (e in 0 until iface.endpointCount) {
                val ep = iface.getEndpoint(e)
                val epInfo = UsbEndpointInfo(
                    endpointNumber = ep.endpointNumber,
                    direction = if (ep.direction == UsbEndpoint.DIR_IN) "IN" else "OUT",
                    transferType = when (ep.type) {
                        UsbEndpoint.TYPE_CONTROL -> "CONTROL"
                        UsbEndpoint.TYPE_ISO -> "ISOCHRONOUS"
                        UsbEndpoint.TYPE_BULK -> "BULK"
                        UsbEndpoint.TYPE_INTERRUPT -> "INTERRUPT"
                        else -> "UNKNOWN"
                    },
                    maxPacketSize = ep.maxPacketSize,
                    interval = ep.interval,
                    address = ep.address
                )

                when {
                    ep.direction == UsbEndpoint.DIR_IN && ep.type == UsbEndpoint.TYPE_BULK -> bulkIn = epInfo
                    ep.direction == UsbEndpoint.DIR_OUT && ep.type == UsbEndpoint.TYPE_BULK -> bulkOut = epInfo
                    ep.direction == UsbEndpoint.DIR_IN && ep.type == UsbEndpoint.TYPE_INTERRUPT -> interruptIn = epInfo
                    ep.direction == UsbEndpoint.DIR_OUT && ep.type == UsbEndpoint.TYPE_INTERRUPT -> interruptOut = epInfo
                }
            }

            var score = 0
            if (bulkIn != null) score += 10
            if (bulkOut != null) score += 10
            if (interruptIn != null) score += 2
            if (interruptOut != null) score += 2
            if (iface.interfaceClass != 0) score += 5

            if (score > bestScore) {
                bestScore = score
                bestLayout = Ca3UsbLayout(
                    interfaceNumber = iface.id,
                    bulkInEndpoint = bulkIn,
                    bulkOutEndpoint = bulkOut,
                    interruptInEndpoint = interruptIn,
                    interruptOutEndpoint = interruptOut
                )
            }
        }

        return bestLayout
    }

    private fun initializeCa3() {
        log("CA3", "INFO", "CA3 initialization - waiting for protocol discovery")
    }

    private suspend fun startReadLoop() {
        val bulkIn = layout?.bulkInEndpoint
        val interruptIn = layout?.interruptInEndpoint

        if (bulkIn == null && interruptIn == null) {
            log("USB", "WARN", "No IN endpoints available for reading")
            return
        }

        scope.launch {
            val buffer = ByteArray(4096)
            var isActive = true
            while (isActive && connection != null) {
                try {
                    if (bulkIn != null) {
                        val read = connection!!.bulkTransfer(
                            device.getInterface(layout!!.interfaceNumber).getEndpoint(bulkIn.endpointNumber),
                            buffer,
                            buffer.size,
                            2000
                        )
                        if (read > 0) {
                            val data = buffer.copyOf(read)
                            onTraffic(TrafficRecord(
                                timestampNs = System.nanoTime(),
                                direction = "CA3_TO_HOST",
                                transferType = "BULK",
                                endpoint = "0x${bulkIn.endpointNumber.toString(16).uppercase()}",
                                payload = data
                            ))
                        }
                    }

                    if (interruptIn != null) {
                        val read = connection!!.bulkTransfer(
                            device.getInterface(layout!!.interfaceNumber).getEndpoint(interruptIn.endpointNumber),
                            buffer,
                            buffer.size,
                            100
                        )
                        if (read > 0) {
                            val data = buffer.copyOf(read)
                            onTraffic(TrafficRecord(
                                timestampNs = System.nanoTime(),
                                direction = "CA3_TO_HOST",
                                transferType = "INTERRUPT",
                                endpoint = "0x${interruptIn.endpointNumber.toString(16).uppercase()}",
                                payload = data
                            ))
                        }
                    }

                    delay(10)
                } catch (e: Exception) {
                    if (isActive) {
                        log("USB", "ERROR", "Read loop error: ${e.message}")
                    }
                    break
                }
            }
        }
    }

    suspend fun writeBulk(data: ByteArray): Boolean {
        if (!activeMode) {
            onError(Ca3Error.TRANSMIT_DISABLED, "Active mode is disabled")
            return false
        }

        val bulkOut = layout?.bulkOutEndpoint ?: return false
        val conn = connection ?: return false

        return try {
            val iface = device.getInterface(layout!!.interfaceNumber)
            val endpoint = iface.getEndpoint(bulkOut.endpointNumber)
            val written = conn.bulkTransfer(endpoint, data, data.size, 2000)
            if (written > 0) {
                onTraffic(TrafficRecord(
                    timestampNs = System.nanoTime(),
                    direction = "HOST_TO_CA3",
                    transferType = "BULK",
                    endpoint = "0x${bulkOut.endpointNumber.toString(16).uppercase()}",
                    payload = data.copyOf(written)
                ))
                true
            } else {
                false
            }
        } catch (e: Exception) {
            log("USB", "ERROR", "Write failed: ${e.message}")
            false
        }
    }

    suspend fun controlTransfer(
        requestType: Int,
        request: Int,
        value: Int,
        index: Int,
        buffer: ByteArray,
        length: Int,
        timeout: Int = 2000
    ): Int {
        val conn = connection ?: return -1
        return try {
            conn.controlTransfer(requestType, request, value, index, buffer, length, timeout)
        } catch (e: Exception) {
            log("USB", "ERROR", "Control transfer failed: ${e.message}")
            -1
        }
    }

    fun setCaptureActive(active: Boolean) {
        captureActive = active
    }

    fun setActiveMode(enabled: Boolean) {
        activeMode = enabled
    }

    fun getLayout(): Ca3UsbLayout? = layout

    fun getDevice(): UsbDevice = device

    fun disconnect() {
        scope.coroutineContext.cancel()
        synchronized(usbMutex) {
            if (claimedInterface != null && connection != null) {
                connection!!.releaseInterface(claimedInterface!!)
                claimedInterface = null
            }
            connection?.close()
            connection = null
        }
        onStateChange(Ca3ConnectionState.DISCONNECTED)
        log("USB", "INFO", "Device disconnected")
    }

    private fun log(module: String, level: String, message: String) {
        onLog(module, level, message)
    }

    data class TrafficRecord(
        val timestampNs: Long,
        val direction: String,
        val transferType: String,
        val endpoint: String,
        val requestType: Int = 0,
        val request: Int = 0,
        val value: Int = 0,
        val index: Int = 0,
        val payload: ByteArray
    )

    data class UsbEndpointInfo(
        val endpointNumber: Int,
        val direction: String,
        val transferType: String,
        val maxPacketSize: Int,
        val interval: Int,
        val address: Int
    )
}