package com.fieldtools.ca3bridge

data class ProtocolHypothesis(
    val type: Int,
    val description: String,
    val confidence: Double,
    val notes: String,
    val sampleFrameHex: String
)