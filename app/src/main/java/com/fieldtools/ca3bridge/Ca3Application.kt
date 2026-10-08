package com.fieldtools.ca3bridge

import android.app.Application
import android.util.Log
import java.io.File
import java.io.PrintWriter
import java.io.StringWriter
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.concurrent.Executors

class Ca3Application : Application() {
    override fun onCreate() {
        super.onCreate()
        Log.i("CA3Bridge", "Application created")
        installCrashHandler()
    }

    private fun installCrashHandler() {
        val defaultHandler = Thread.getDefaultUncaughtExceptionHandler()
        Thread.setDefaultUncaughtExceptionHandler { thread, ex ->
            try {
                Log.e("CA3Crash", "CRASH in ${thread.name}: ", ex)
                // Write full crash report to app files directory
                val crashDir = File(filesDir, "crash_logs")
                crashDir.mkdirs()
                val sw = StringWriter()
                sw.write("CRASH ${SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.US).format(Date())} in thread ${thread.name}\n")
                sw.write("Process: ${android.os.Process.myPid()}\n")
                PrintWriter(sw).use { out ->
                    ex.printStackTrace(out)
                }
                val crashFile = File(crashDir, "crash_${System.currentTimeMillis()}.txt")
                crashFile.writeText(sw.toString())
                Log.e("CA3Crash", "Crash report written to: ${crashFile.absolutePath}")

                // Also write to external storage for easy retrieval
                try {
                    val extDir = File(getExternalFilesDir(null), "crash_logs")
                    extDir.mkdirs()
                    File(extDir, "crash_${System.currentTimeMillis()}.txt").writeText(sw.toString())
                } catch (e: Exception) {
                    Log.w("CA3Crash", "Failed to write external crash log", e)
                }

                // Show crash details to user via toast so they can report it
                try {
                    runOnUiThread {
                        val msg = "CRASH: ${ex::class.java.simpleName}: ${ex.message}\n" +
                            "First lines:\n" +
                            ex.stackTrace.take(3).joinToString("\n") { "  at ${it}" }
                        Log.e("CA3Crash", msg)
                    }
                } catch (_: Exception) {}
            } catch (e: Exception) {
                Log.e("CA3Crash", "Failed to handle crash", e)
            }
            // Delegate to default handler to actually crash (or let system handle it)
            defaultHandler?.uncaughtException(thread, ex) ?: run {
                // No default handler - just die
            }
        }
    }
}
