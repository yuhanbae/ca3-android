package com.fieldtools.ca3bridge

data class FrameAnalysis(
    val payload: ByteArray,
    val hypotheses: List<String>,
    val lengthFieldPos: Int,
    val cmdFieldPos: Int,
    val crcInfo: String?
)