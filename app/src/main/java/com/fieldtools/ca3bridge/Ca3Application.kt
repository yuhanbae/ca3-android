package com.fieldtools.ca3bridge

import android.app.Application
import android.util.Log

class Ca3Application : Application() {
    override fun onCreate() {
        super.onCreate()
        Log.i("CA3Bridge", "Application created")
    }
}