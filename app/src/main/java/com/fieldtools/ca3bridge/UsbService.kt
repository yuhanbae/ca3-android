package com.fieldtools.ca3bridge

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import android.os.Build
import android.os.IBinder
import android.util.Log
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch

class UsbService : Service() {
    private var usbManager: UsbManager? = null
    private var usbReceiver: BroadcastReceiver? = null
    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())

    override fun onCreate() {
        super.onCreate()
        usbManager = getSystemService(Context.USB_SERVICE) as UsbManager

        createNotificationChannel()
        val notification = buildNotification()
        val foregroundType: Int
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            foregroundType = try {
                Service::class.java.getField("FOREGROUND_SERVICE_TYPE_DATA_SYNC").getInt(null)
            } catch (e: Exception) {
                0
            }
        } else {
            foregroundType = 0
        }
        startForeground(1, notification, foregroundType)

        usbReceiver = object : BroadcastReceiver() {
            override fun onReceive(context: Context, intent: Intent) {
                val action = intent.action
                when (action) {
                    UsbManager.ACTION_USB_DEVICE_ATTACHED -> {
                        val device = intent.getParcelableExtra<UsbDevice>(UsbManager.EXTRA_DEVICE)
                        device?.let { Log.i("CA3Bridge", "USB device attached: ${it.deviceId}") }
                    }
                    UsbManager.ACTION_USB_DEVICE_DETACHED -> {
                        val device = intent.getParcelableExtra<UsbDevice>(UsbManager.EXTRA_DEVICE)
                        device?.let { Log.i("CA3Bridge", "USB device detached: ${it.deviceId}") }
                    }
                }
            }
        }

        val filter = IntentFilter().apply {
            addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED)
            addAction(UsbManager.ACTION_USB_DEVICE_DETACHED)
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(usbReceiver, filter, Context.RECEIVER_EXPORTED)
        } else {
            registerReceiver(usbReceiver, filter)
        }
    }

    override fun onDestroy() {
        usbReceiver?.let { unregisterReceiver(it) }
        scope.cancel()
        stopForeground(true)
        super.onDestroy()
    }

    override fun onBind(intent: Intent): IBinder? = null

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                "ca3_usb_channel",
                "CA3 USB Service",
                NotificationManager.IMPORTANCE_LOW
            ).apply {
                description = "Background USB monitoring for CA3 Bridge"
            }
            val manager = getSystemService(NotificationManager::class.java)
            manager.createNotificationChannel(channel)
        }
    }

    private fun buildNotification(): Notification {
        val intent = Intent(this, MainActivity::class.java).apply {
            flags = Intent.FLAG_ACTIVITY_SINGLE_TOP
        }
        val flags = PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
        val pendingIntent = PendingIntent.getActivity(this, 0, intent, flags)

        val builder = Notification.Builder(this, "ca3_usb_channel")
            .setContentTitle("CA3 Bridge")
            .setContentText("Monitoring USB for CA3 device")
            .setContentIntent(pendingIntent)
            .setOngoing(true)

        builder.setSmallIcon(android.R.drawable.ic_menu_directions)

        return builder.build()
    }
}