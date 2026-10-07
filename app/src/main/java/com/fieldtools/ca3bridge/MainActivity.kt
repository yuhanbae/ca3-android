package com.fieldtools.ca3bridge

import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class MainActivity : AppCompatActivity() {
    
    private lateinit var usbManager: UsbManager
    private lateinit var permissionIntent: PendingIntent
    private var ca3Device: Ca3Device? = null
    private var protocolLogger: ProtocolLogger? = null
    private var selectedDevice: UsbDevice? = null
    private var deviceList: List<UsbDevice> = emptyList()
    private var activeMode = false
    private var readOnlyMode = true
    
    private val usbReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val action = intent.action
            when (action) {
                "android.hardware.usb.action.USB_PERMISSION" -> {
                    val device = intent.getParcelableExtra<UsbDevice>(UsbManager.EXTRA_DEVICE)
                    val granted = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)
                    device?.let {
                        if (granted) {
                            log("USB", "INFO", "Permission granted for ${it.deviceId}")
                            connectToDevice(it)
                        } else {
                            log("USB", "ERROR", "Permission denied for ${it.deviceId}")
                            updateStatus("PERMISSION DENIED", error = true)
                        }
                    }
                }
                UsbManager.ACTION_USB_DEVICE_ATTACHED -> {
                    refreshDeviceList()
                }
                UsbManager.ACTION_USB_DEVICE_DETACHED -> {
                    val device = intent.getParcelableExtra<UsbDevice>(UsbManager.EXTRA_DEVICE)
                    device?.let {
                        if (it.deviceId == selectedDevice?.deviceId) {
                            disconnect()
                        }
                    }
                    refreshDeviceList()
                }
            }
        }
    
    override fun onCreate(savedInstanceState: Bundle?) {
        try {
            super.onCreate(savedInstanceState)
            setContentView(R.layout.activity_main)
        } catch (e: Exception) {
            Toast.makeText(this, "Startup error: ${e.message}", Toast.LENGTH_LONG).show()
            finish()
        }
        
        usbManager = getSystemService(Context.USB_SERVICE) as UsbManager

        // Check for USB host support - crash protection
        if (!getPackageManager().hasSystemFeature(PackageManager.FEATURE_USB_HOST)) {
            runOnUiThread {
                updateStatus("NO USB HOST SUPPORT", error = true)
                toast("This device does not support USB Host mode")
            }
            finish()
            return
        }
        permissionIntent = PendingIntent.getBroadcast(
            this, 0, Intent("com.fieldtools.ca3bridge.USB_PERMISSION"),
            PendingIntent.FLAG_IMMUTABLE
        )
        
        protocolLogger = ProtocolLogger(this) { module, level, message ->
            runOnUiThread { appendLog(module, level, message) }
        }
        
        setupUI()
        refreshDeviceList()
        log("APP", "INFO", "CA3 Bridge started")
    }
    
    override fun onResume() {
        super.onResume()
        registerReceiver(usbReceiver, IntentFilter().apply {
            addAction("android.hardware.usb.action.USB_PERMISSION")
            addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED)
            addAction(UsbManager.ACTION_USB_DEVICE_DETACHED)
        }, Context.RECEIVER_EXPORTED)
    }
    
    override fun onPause() {
        super.onPause()
        try { unregisterReceiver(usbReceiver) } catch (e: Exception) {}
    }
    
    override fun onDestroy() {
        ca3Device?.disconnect()
        super.onDestroy()
    }
    
    private fun setupUI() {
        findViewById<View>(R.id.refreshBtn).setOnClickListener { refreshDeviceList() }
        
        findViewById<View>(R.id.permissionBtn).setOnClickListener {
            selectedDevice?.let { requestPermission(it) }
        }
        
        findViewById<View>(R.id.connectBtn).setOnClickListener {
            selectedDevice?.let { connectToDevice(it) }
        }
        
        findViewById<View>(R.id.disconnectBtn).setOnClickListener { disconnect() }
        
        findViewById<View>(R.id.exportDescriptorBtn).setOnClickListener { exportDescriptor() }
        
        findViewById<View>(R.id.captureBtn).setOnClickListener { toggleCapture() }
        
        findViewById<View>(R.id.activeModeBtn).setOnClickListener { toggleActiveMode() }
    }
    
    private fun refreshDeviceList() {
        lifecycleScope.launch {
            val devices = withContext(Dispatchers.IO) {
                usbManager.deviceList.values.toList()
            }
            runOnUiThread {
                deviceList = it
                updateDeviceList(it)
            }
        }
    }
    
    private fun updateDeviceList(devices: List<UsbDevice>) {
        deviceList = devices
        val recyclerView = findViewById<RecyclerView>(R.id.deviceList)
        recyclerView.layoutManager = LinearLayoutManager(this@MainActivity)
        recyclerView.adapter = DeviceAdapter(devices, this@MainActivity) { device ->
            selectedDevice = it
            updateSelectionUI(it)
        }
    }
    
    private fun updateSelectionUI(device: UsbDevice) {
        DeviceAdapter.selectedDeviceId = device.deviceId
        findViewById<TextView>(R.id.vidValue).text = String.format("0x%04X", device.vendorId)
        findViewById<TextView>(R.id.pidValue).text = String.format("0x%04X", device.productId)
        findViewById<TextView>(R.id.productValue).text = device.productName ?: "Unknown"
        
        val hasPermission = usbManager.hasPermission(device)
        findViewById<View>(R.id.permissionBtn).isEnabled = !hasPermission
        findViewById<View>(R.id.connectBtn).isEnabled = hasPermission
        findViewById<View>(R.id.exportDescriptorBtn).isEnabled = true
        
        (findViewById<RecyclerView>(R.id.deviceList).adapter as? DeviceAdapter)?.notifyDataSetChanged()
    }
    
    private fun requestPermission(device: UsbDevice) {
        updateStatus("PERMISSION REQUIRED")
        usbManager.requestPermission(device, permissionIntent)
    }
    
    private fun connectToDevice(device: UsbDevice) {
        updateStatus("CONNECTING...")
        findViewById<View>(R.id.connectBtn).isEnabled = false
        findViewById<View>(R.id.disconnectBtn).isEnabled = false
        
        ca3Device = Ca3Device(
            usbManager = usbManager,
            device = device,
            onStateChange = { state -> runOnUiThread { onCa3StateChange(it) } },
            onError = { error, msg -> runOnUiThread { onCa3Error(it, msg) } },
            onLog = { module, level, msg -> runOnUiThread { appendLog(module, level, msg) } },
            onTraffic = { record -> 
                protocolLogger?.record(it)
                runOnUiThread { appendTraffic(it) }
            }
        )
        
        ca3Device!!.requestPermission(permissionIntent)
    }
    
    private fun onCa3StateChange(state: Ca3ConnectionState) {
        when (state) {
            Ca3ConnectionState.USB_CONNECTED -> updateStatus("PERMISSION GRANTED")
            Ca3ConnectionState.DESCRIPTOR_READY -> updateStatus("DESCRIPTOR READY")
            Ca3ConnectionState.USB_CLAIMED -> {
                updateStatus("INTERFACE CLAIMED")
                showInterfaceInfo()
            }
            Ca3ConnectionState.CA3_READY -> {
                updateStatus("CONNECTED", connected = true)
                findViewById<View>(R.id.disconnectBtn).isEnabled = true
                findViewById<View>(R.id.captureBtn).isEnabled = true
                findViewById<View>(R.id.activeModeBtn).isEnabled = true
            }
            Ca3ConnectionState.ERROR -> {
                updateStatus("ERROR", error = true)
                findViewById<View>(R.id.connectBtn).isEnabled = true
            }
            Ca3ConnectionState.DISCONNECTED -> {
                updateStatus("DISCONNECTED")
                findViewById<View>(R.id.connectBtn).isEnabled = true
                findViewById<View>(R.id.disconnectBtn).isEnabled = false
                findViewById<View>(R.id.captureBtn).isEnabled = false
                findViewById<View>(R.id.activeModeBtn).isEnabled = false
                findViewById<View>(R.id.exportDescriptorBtn).isEnabled = false
                findViewById<View>(R.id.interfaceCard).visibility = View.GONE
            }
        }
    }
    
    private fun onCa3Error(error: Ca3Error, message: String) {
        log("USB", "ERROR", "$error: $message")
        updateStatus("ERROR: $message", error = true)
    }
    
    private fun showInterfaceInfo() {
        ca3Device?.let { device ->
            val layout = device.getLayout()
            it?.let {
                val info = StringBuilder()
                info.append("Interface: ${it.interfaceNumber}\n")
                it.bulkInEndpoint?.let { info.append("BULK IN:  EP=0x${it.endpointNumber.toString(16).uppercase()} MAX=${it.maxPacketSize}\n") }
                it.bulkOutEndpoint?.let { info.append("BULK OUT: EP=0x${it.endpointNumber.toString(16).uppercase()} MAX=${it.maxPacketSize}\n") }
                it.interruptInEndpoint?.let { info.append("INT IN:   EP=0x${it.endpointNumber.toString(16).uppercase()} MAX=${it.maxPacketSize}\n") }
                it.interruptOutEndpoint?.let { info.append("INT OUT:  EP=0x${it.endpointNumber.toString(16).uppercase()} MAX=${it.maxPacketSize}\n") }
                
                findViewById<TextView>(R.id.interfaceInfo).text = info.toString()
                findViewById<View>(R.id.interfaceCard).visibility = View.VISIBLE
            }
        }
    }
    
    private fun disconnect() {
        ca3Device?.disconnect()
        ca3Device = null
        selectedDevice = null
        DeviceAdapter.selectedDeviceId = -1
        findViewById<TextView>(R.id.vidValue).text = "\u2014"
        findViewById<TextView>(R.id.pidValue).text = "\u2014"
        findViewById<TextView>(R.id.productValue).text = "\u2014"
        findViewById<View>(R.id.interfaceCard).visibility = View.GONE
    }
    
    private fun exportDescriptor() {
        ca3Device?.let { device ->
            lifecycleScope.launch {
                val file = withContext(Dispatchers.IO) { protocolLogger?.exportDescriptor(it) }
                it?.let { runOnUiThread { toast("Descriptor exported: ${it.name}") } }
                    ?: runOnUiThread { toast("Export failed") }
            }
        }
    }
    
    private fun toggleCapture() {
        val btn = findViewById<View>(R.id.captureBtn) as com.google.android.material.button.MaterialButton
        val capturing = protocolLogger?.isCapturing() ?: false
        
        if (capturing) {
            val summary = protocolLogger?.stopCapture()
            summary?.let { toast("Capture stopped: ${it.recordCount} records") }
            btn.text = "Start Capture"
            findViewById<TextView>(R.id.captureStatus).text = "IDLE"
        } else {
            if (protocolLogger?.startCapture() == true) {
                btn.text = "Stop Capture"
                findViewById<TextView>(R.id.captureStatus).text = "CAPTURING"
                toast("Capture started")
            }
        }
    }
    
    private fun toggleActiveMode() {
        activeMode = !activeMode
        readOnlyMode = !activeMode
        ca3Device?.setActiveMode(activeMode)
        val btn = findViewById<View>(R.id.activeModeBtn) as com.google.android.material.button.MaterialButton
        btn.text = if (activeMode) "Disable Active Mode" else "Enable Active Mode"
        log("APP", if (activeMode) "WARN" else "INFO", "Active mode ${if (activeMode) "ENABLED" else "DISABLED"}")
    }
    
    private fun updateStatus(text: String, connected: Boolean = false, error: Boolean = false) {
        val label = findViewById<TextView>(R.id.statusLabel)
        val indicator = findViewById<TextView>(R.id.statusIndicator)
        
        label.text = text
        if (connected) {
            label.setTextColor(ContextCompat.getColor(this@MainActivity, R.color.ok_green))
            indicator.setTextColor(ContextCompat.getColor(this@MainActivity, R.color.ok_green))
        } else if (error) {
            label.setTextColor(ContextCompat.getColor(this@MainActivity, R.color.error_red))
            indicator.setTextColor(ContextCompat.getColor(this@MainActivity, R.color.error_red))
        } else {
            label.setTextColor(ContextCompat.getColor(this@MainActivity, R.color.warn_amber))
            indicator.setTextColor(ContextCompat.getColor(this@MainActivity, R.color.warn_amber))
        }
    }
    
    private fun appendLog(module: String, level: String, message: String) {
        val console = findViewById<TextView>(R.id.logConsole)
        val time = java.text.SimpleDateFormat("HH:mm:ss.SSS").format(java.util.Date())
        val color = when (level) {
            "ERROR" -> "#EF4444"
            "WARN" -> "#F59E0B"
            "DEBUG" -> "#86EFAC"
            else -> "#E9EEF3"
        }
        console.append("\n[$time] [$level] [$module] $message")
        val lines = console.text.toString().split("\n")
        if (lines.size > 200) {
            console.text = lines.drop(lines.size - 200).joinToString("\n")
        }
    }
    
    private fun appendTraffic(record: Ca3Device.TrafficRecord) {
        val console = findViewById<TextView>(R.id.trafficConsole)
        val dirArrow = if (record.direction == "HOST_TO_CA3") "\u25B6" else "\u25C0"
        val hex = record.payload.joinToString(" ") { String.format("%02X", it) }
        val preview = if (hex.length > 120) hex.substring(0, 120) + "..." else hex
        console.append("\n$dirArrow ${record.transferType} EP=${record.endpoint} [${record.payload.size}B] $preview")
        val lines = console.text.toString().split("\n")
        if (lines.size > 150) {
            console.text = lines.drop(lines.size - 150).joinToString("\n")
        }
    }
    
    private fun log(module: String, level: String, message: String) {
        appendLog(module, level, message)
        Log.i("CA3Bridge", "[$module] [$level] $message")
    }
    
    private fun toast(msg: String) {
        Toast.makeText(this@MainActivity, msg, Toast.LENGTH_SHORT).show()
    }
}
}