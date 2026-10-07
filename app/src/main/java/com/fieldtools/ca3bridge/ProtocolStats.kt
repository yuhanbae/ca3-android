package com.fieldtools.ca3bridge

data class ProtocolStats(
    val usbPacketsReceived: Long,
    val usbPacketsSent: Long,
    val j1939FramesDecoded: Long,
    val isoTpFramesDecoded: Long
)