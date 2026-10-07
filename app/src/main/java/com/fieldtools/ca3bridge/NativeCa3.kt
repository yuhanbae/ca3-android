package com.fieldtools.ca3bridge

class NativeCa3 {

    companion object {
        @Volatile
        var nativeAvailable = false

        init {
            try {
                System.loadLibrary("ca3native")
                nativeAvailable = true
            } catch (_: UnsatisfiedLinkError) {
                nativeAvailable = false
            }
        }
    }

    private var handle: Long = 0L

    fun create() {
        check(handle == 0L) { "Native bridge already created" }
        if (!companionObject.nativeAvailable) {
            handle = 1L
            return
        }
        handle = nativeCreate()
        check(handle != 0L) { "nativeCreate() failed" }
    }

    fun destroy() {
        if (handle != 0L) {
            if (companionObject.nativeAvailable) {
                nativeDestroy(handle)
            }
            handle = 0L
        }
    }

    // Feed raw USB data (device to host)
    fun feed(data: ByteArray) {
        check(handle != 0L) { "Native bridge not created" }
        if (companionObject.nativeAvailable) {
            nativeFeed(handle, data)
        }
    }

    // Feed raw USB data with direction and timestamp
    fun feedWithDirection(data: ByteArray, hostToDevice: Boolean, timestampNs: Long) {
        check(handle != 0L) { "Native bridge not created" }
        if (companionObject.nativeAvailable) {
            nativeFeedWithDirection(handle, data, hostToDevice, timestampNs)
        }
    }

    // Find frame candidates in the buffer
    fun findCandidates(): List<FrameCandidate>? {
        check(handle != 0L) { "Native bridge not created" }
        return if (companionObject.nativeAvailable) {
            val array = nativeFindCandidates(handle)
            if (array != null) array.toList() else null
        } else null
    }

    // Analyze a specific frame for patterns
    fun analyzeFrame(data: ByteArray): FrameAnalysis? {
        check(handle != 0L) { "Native bridge not created" }
        return if (companionObject.nativeAvailable) {
            nativeAnalyzeFrame(handle, data)
        } else null
    }

    // Get decoded J1939 frames
    fun getJ1939Frames(): List<J1939Frame>? {
        check(handle != 0L) { "Native bridge not created" }
        return if (companionObject.nativeAvailable) {
            val array = nativeGetJ1939Frames(handle)
            if (array != null) array.toList() else null
        } else null
    }

    // Get protocol hypotheses
    fun getHypotheses(): List<ProtocolHypothesis>? {
        check(handle != 0L) { "Native bridge not created" }
        return if (companionObject.nativeAvailable) {
            val array = nativeGetHypotheses(handle)
            if (array != null) array.toList() else null
        } else null
    }

    // Clear all analysis state
    fun clear() {
        check(handle != 0L) { "Native bridge not created" }
        if (companionObject.nativeAvailable) {
            nativeClear(handle)
        }
    }

    // Get protocol statistics
    fun getStats(): ProtocolStats? {
        check(handle != 0L) { "Native bridge not created" }
        return if (companionObject.nativeAvailable) {
            nativeGetStats(handle)
        } else null
    }

    // Test all CRC variants against data
    fun testCrc(data: ByteArray, expectedCrc: Long): List<CrcResult>? {
        check(handle != 0L) { "Native bridge not created" }
        return if (companionObject.nativeAvailable) {
            val array = nativeTestCrc(handle, data, expectedCrc)
            if (array != null) array.toList() else null
        } else null
    }

    // JNI native methods
    private external fun nativeCreate(): Long
    private external fun nativeDestroy(handle: Long)
    private external fun nativeFeed(handle: Long, data: ByteArray)
    private external fun nativeFeedWithDirection(handle: Long, data: ByteArray, hostToDevice: Boolean, timestampNs: Long)
    private external fun nativeFindCandidates(handle: Long): Array<FrameCandidate>?
    private external fun nativeAnalyzeFrame(handle: Long, data: ByteArray): FrameAnalysis?
    private external fun nativeGetJ1939Frames(handle: Long): Array<J1939Frame>?
    private external fun nativeGetHypotheses(handle: Long): Array<ProtocolHypothesis>?
    private external fun nativeClear(handle: Long)
    private external fun nativeGetStats(handle: Long): ProtocolStats?
    private external fun nativeTestCrc(handle: Long, data: ByteArray, expectedCrc: Long): Array<CrcResult>?
}