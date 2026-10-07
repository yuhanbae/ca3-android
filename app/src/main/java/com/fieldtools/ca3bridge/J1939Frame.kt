package com.fieldtools.ca3bridge

data class J1939Frame(
    val canId: Int,
    val data: ByteArray,
    val dlc: Byte,
    val pgn: Int,
    val timestampNs: Long
)