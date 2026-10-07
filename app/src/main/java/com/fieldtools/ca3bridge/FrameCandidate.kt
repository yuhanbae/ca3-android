package com.fieldtools.ca3bridge

data class FrameCandidate(
    val offset: Int,
    val length: Int,
    val possibleStartByte: Int,
    val possibleLengthByte: Int,
    val hasValidCrc: Boolean,
    val crcType: String,
    val confidence: Double,
    val hypothesis: String
)