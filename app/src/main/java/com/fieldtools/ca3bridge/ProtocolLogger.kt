package com.fieldtools.ca3bridge

import android.content.Context
import android.util.Log
import com.google.gson.Gson
import com.google.gson.GsonBuilder
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File
import java.io.FileOutputStream
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.concurrent.atomic.AtomicLong

data class CaptureRecord(
    val id: Long = 0,
    val timestampNs: Long,
    val direction: String,
    val transferType: String,
    val endpoint: String,
    val requestType: Int,
    val request: Int,
    val value: Int,
    val index: Int,
    val payload: ByteArray
)

class ProtocolLogger(
    private val context: Context,
    private val onLog: (String, String, String) -> Unit
) {
    private var captureFile: File? = null
    private var captureOutput: FileOutputStream? = null
    private var isCapturing = false
    private val recordCount = AtomicLong(0)
    private val dbHelper: CaptureDatabase
    private val gson = GsonBuilder().create()

    init {
        dbHelper = CaptureDatabase(context)
    }

    fun startCapture(): Boolean {
        if (isCapturing) return false

        try {
            val timestamp = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(Date())
            val dir = File(context.filesDir, "captures")
            dir.mkdirs()
            captureFile = File(dir, "ca3_capture_$timestamp.c3cap")
            captureOutput = FileOutputStream(captureFile!!)

            val header = "C3CAPv1\n".toByteArray()
            captureOutput!!.write(header)

            isCapturing = true
            recordCount.set(0)
            log("CAPTURE", "INFO", "Capture started: ${captureFile!!.name}")
            return true
        } catch (e: Exception) {
            log("CAPTURE", "ERROR", "Failed to start capture: ${e.message}")
            return false
        }
    }

    fun stopCapture(): CaptureSummary? {
        if (!isCapturing) return null

        try {
            captureOutput?.close()
            captureOutput = null
            isCapturing = false
            val count = recordCount.get()
            val file = captureFile
            captureFile = null
            log("CAPTURE", "INFO", "Capture stopped: $count records")
            return CaptureSummary(file!!, count)
        } catch (e: Exception) {
            log("CAPTURE", "ERROR", "Failed to stop capture: ${e.message}")
            return null
        }
    }

    data class CaptureSummary(val file: File, val recordCount: Long)

    suspend fun record(record: Ca3Device.TrafficRecord) {
        if (!isCapturing) return

        withContext(Dispatchers.IO) {
            try {
                val payload = record.payload
                val header = ByteArray(32)
                var offset = 0
                header[offset++] = (record.timestampNs shr 56).toByte()
                header[offset++] = (record.timestampNs shr 48).toByte()
                header[offset++] = (record.timestampNs shr 40).toByte()
                header[offset++] = (record.timestampNs shr 32).toByte()
                header[offset++] = (record.timestampNs shr 24).toByte()
                header[offset++] = (record.timestampNs shr 16).toByte()
                header[offset++] = (record.timestampNs shr 8).toByte()
                header[offset++] = (record.timestampNs).toByte()
                header[offset++] = if (record.direction == "HOST_TO_CA3") 0 else 1
                header[offset++] = when (record.transferType) {
                    "CONTROL" -> 0.toByte()
                    "BULK" -> 1.toByte()
                    "INTERRUPT" -> 2.toByte()
                    "ISOCHRONOUS" -> 3.toByte()
                    else -> 255.toByte()
                }
                header[offset++] = record.endpoint.replace("0x", "").toIntOrNull(16)?.toByte() ?: 0
                header[offset++] = record.requestType.toByte()
                header[offset++] = record.request.toByte()
                header[offset++] = (record.value shr 8).toByte()
                header[offset++] = record.value.toByte()
                header[offset++] = (record.index shr 8).toByte()
                header[offset++] = record.index.toByte()
                header[offset++] = (payload.size shr 24).toByte()
                header[offset++] = (payload.size shr 16).toByte()
                header[offset++] = (payload.size shr 8).toByte()
                header[offset++] = payload.size.toByte()

                captureOutput?.write(header)
                captureOutput?.write(payload)
                captureOutput?.flush()

                recordCount.incrementAndGet()
            } catch (e: Exception) {
                log("CAPTURE", "ERROR", "Record write failed: ${e.message}")
            }
        }

        dbHelper.insertRecord(CaptureRecord(
            timestampNs = record.timestampNs,
            direction = record.direction,
            transferType = record.transferType,
            endpoint = record.endpoint,
            requestType = record.requestType,
            request = record.request,
            value = record.value,
            index = record.index,
            payload = record.payload
        ))
    }

    fun exportDescriptor(device: Ca3Device): File? {
        try {
            val timestamp = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(Date())
            val dir = File(context.filesDir, "descriptors")
            dir.mkdirs()
            val file = File(dir, "ca3_descriptor_$timestamp.json")

            val layout = device.getLayout()
            val json = gson.toJson(DeviceDescriptorExport(
                timestampNs = System.nanoTime(),
                vendorId = device.getDevice().vendorId,
                productId = device.getDevice().productId,
                deviceClass = device.getDevice().deviceClass,
                deviceSubclass = device.getDevice().deviceSubclass,
                deviceProtocol = device.getDevice().deviceProtocol,
                manufacturerName = device.getDevice().manufacturerName,
                productName = device.getDevice().productName,
                serialNumber = device.getDevice().serialNumber,
                interfaceNumber = layout?.interfaceNumber ?: -1,
                bulkInEndpoint = layout?.bulkInEndpoint?.let { Ca3Device.UsbEndpointInfo(
                    it.endpointNumber, it.direction, it.transferType, it.maxPacketSize, it.interval, it.address
                ) },
                bulkOutEndpoint = layout?.bulkOutEndpoint?.let { Ca3Device.UsbEndpointInfo(
                    it.endpointNumber, it.direction, it.transferType, it.maxPacketSize, it.interval, it.address
                ) },
                interruptInEndpoint = layout?.interruptInEndpoint?.let { Ca3Device.UsbEndpointInfo(
                    it.endpointNumber, it.direction, it.transferType, it.maxPacketSize, it.interval, it.address
                ) },
                interruptOutEndpoint = layout?.interruptOutEndpoint?.let { Ca3Device.UsbEndpointInfo(
                    it.endpointNumber, it.direction, it.transferType, it.maxPacketSize, it.interval, it.address
                ) }
            ))

            file.writeText(json)
            log("CAPTURE", "INFO", "Descriptor exported: ${file.name}")
            return file
        } catch (e: Exception) {
            log("CAPTURE", "ERROR", "Descriptor export failed: ${e.message}")
            return null
        }
    }

    data class DeviceDescriptorExport(
        val timestampNs: Long,
        val vendorId: Int,
        val productId: Int,
        val deviceClass: Int,
        val deviceSubclass: Int,
        val deviceProtocol: Int,
        val manufacturerName: String?,
        val productName: String?,
        val serialNumber: String?,
        val interfaceNumber: Int,
        val bulkInEndpoint: Ca3Device.UsbEndpointInfo?,
        val bulkOutEndpoint: Ca3Device.UsbEndpointInfo?,
        val interruptInEndpoint: Ca3Device.UsbEndpointInfo?,
        val interruptOutEndpoint: Ca3Device.UsbEndpointInfo?
    )

    fun isCapturing(): Boolean = isCapturing

    fun getRecordCount(): Long = recordCount.get()

    private fun log(module: String, level: String, message: String) {
        onLog(module, level, message)
    }
}

class CaptureDatabase(private val context: Context) {
    private val dbPath = File(context.filesDir, "ca3_captures.db")

    init {
        initDatabase()
    }

    private fun initDatabase() {
        try {
            val conn = java.sql.DriverManager.getConnection("jdbc:sqlite:${dbPath.path}")
            conn.createStatement().execute("""
                CREATE TABLE IF NOT EXISTS usb_capture (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    timestamp_ns INTEGER NOT NULL,
                    direction TEXT NOT NULL,
                    transfer_type TEXT NOT NULL,
                    endpoint TEXT NOT NULL,
                    request_type INTEGER NOT NULL,
                    request INTEGER NOT NULL,
                    value INTEGER NOT NULL,
                    index_value INTEGER NOT NULL,
                    payload BLOB NOT NULL
                )
            """.trimIndent())
            conn.createStatement().execute("""
                CREATE INDEX IF NOT EXISTS idx_capture_timestamp ON usb_capture(timestamp_ns)
            """.trimIndent())
            conn.createStatement().execute("""
                CREATE INDEX IF NOT EXISTS idx_capture_direction ON usb_capture(direction)
            """.trimIndent())
            conn.close()
        } catch (e: Exception) {
            Log.e("CA3Bridge", "Database init failed: ${e.message}")
        }
    }

    suspend fun insertRecord(record: CaptureRecord) {
        withContext(Dispatchers.IO) {
            try {
                val conn = java.sql.DriverManager.getConnection("jdbc:sqlite:${dbPath.path}")
                val stmt = conn.prepareStatement(
                    "INSERT INTO usb_capture (timestamp_ns, direction, transfer_type, endpoint, request_type, request, value, index_value, payload) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"
                )
                stmt.setLong(1, record.timestampNs)
                stmt.setString(2, record.direction)
                stmt.setString(3, record.transferType)
                stmt.setString(4, record.endpoint)
                stmt.setInt(5, record.requestType)
                stmt.setInt(6, record.request)
                stmt.setInt(7, record.value)
                stmt.setInt(8, record.index)
                stmt.setBytes(9, record.payload)
                stmt.executeUpdate()
                stmt.close()
                conn.close()
            } catch (e: Exception) {
                Log.e("CA3Bridge", "Database insert failed: ${e.message}")
            }
        }
    }

    suspend fun queryRecords(limit: Int = 1000, offset: Int = 0): List<CaptureRecord> {
        return withContext(Dispatchers.IO) {
            try {
                val conn = java.sql.DriverManager.getConnection("jdbc:sqlite:${dbPath.path}")
                val stmt = conn.prepareStatement(
                    "SELECT id, timestamp_ns, direction, transfer_type, endpoint, request_type, request, value, index_value, payload FROM usb_capture ORDER BY timestamp_ns DESC LIMIT ? OFFSET ?"
                )
                stmt.setInt(1, limit)
                stmt.setInt(2, offset)
                val rs = stmt.executeQuery()
                val results = mutableListOf<CaptureRecord>()
                while (rs.next()) {
                    results.add(CaptureRecord(
                        id = rs.getLong("id"),
                        timestampNs = rs.getLong("timestamp_ns"),
                        direction = rs.getString("direction") ?: "",
                        transferType = rs.getString("transfer_type") ?: "",
                        endpoint = rs.getString("endpoint") ?: "",
                        requestType = rs.getInt("request_type"),
                        request = rs.getInt("request"),
                        value = rs.getInt("value"),
                        index = rs.getInt("index_value"),
                        payload = rs.getBytes("payload") ?: ByteArray(0)
                    ))
                }
                rs.close()
                stmt.close()
                conn.close()
                results
            } catch (e: Exception) {
                Log.e("CA3Bridge", "Database query failed: ${e.message}")
                emptyList()
            }
        }
    }

    suspend fun clearDatabase() {
        withContext(Dispatchers.IO) {
            try {
                val conn = java.sql.DriverManager.getConnection("jdbc:sqlite:${dbPath.path}")
                conn.createStatement().execute("DELETE FROM usb_capture")
                conn.close()
            } catch (e: Exception) {
                Log.e("CA3Bridge", "Database clear failed: ${e.message}")
            }
        }
    }
}