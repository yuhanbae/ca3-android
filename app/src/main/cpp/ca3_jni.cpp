#include <jni.h>
#include <android/log.h>
#include <memory>
#include <vector>
#include <string>
#include "ca3_protocol.hpp"
#include "ca3_frame.hpp"
#include "ca3_crc.hpp"

#define LOG_TAG "CA3Native"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

// Handle management
struct NativeHandle {
    std::unique_ptr<ca3::FrameAnalyzer> frame_analyzer;
    std::unique_ptr<ca3::ProtocolAnalyzer> protocol_analyzer;
};

static NativeHandle* getHandle(jlong handle) {
    return reinterpret_cast<NativeHandle*>(handle);
}

// JNI function signatures
extern "C" {

JNIEXPORT jlong JNICALL
Java_com_yuhanbae_ca3_NativeCa3_nativeCreate(JNIEnv* env, jobject thiz) {
    auto handle = new NativeHandle();
    handle->frame_analyzer = std::make_unique<ca3::FrameAnalyzer>();
    handle->protocol_analyzer = std::make_unique<ca3::ProtocolAnalyzer>();
    LOGI("Native handle created: %p", handle);
    return reinterpret_cast<jlong>(handle);
}

JNIEXPORT void JNICALL
Java_com_yuhanbae_ca3_NativeCa3_nativeDestroy(JNIEnv* env, jobject thiz, jlong handle) {
    NativeHandle* h = getHandle(handle);
    if (h) {
        delete h;
        LOGI("Native handle destroyed");
    }
}

JNIEXPORT void JNICALL
Java_com_yuhanbae_ca3_NativeCa3_nativeFeed(JNIEnv* env, jobject thiz, jlong handle, jbyteArray data) {
    NativeHandle* h = getHandle(handle);
    if (!h || !h->frame_analyzer || !data) return;
    
    jsize len = env->GetArrayLength(data);
    if (len <= 0) return;
    
    std::vector<jbyte> buffer(len);
    env->GetByteArrayRegion(data, 0, len, buffer.data());
    
    h->frame_analyzer->feed(reinterpret_cast<const uint8_t*>(buffer.data()), len);
    h->protocol_analyzer->feedUsbPayload(reinterpret_cast<const uint8_t*>(buffer.data()), len, 0, false);
}

JNIEXPORT void JNICALL
Java_com_yuhanbae_ca3_NativeCa3_nativeFeedWithDirection(JNIEnv* env, jobject thiz, jlong handle, jbyteArray data, jboolean host_to_device, jlong timestamp) {
    NativeHandle* h = getHandle(handle);
    if (!h || !h->frame_analyzer || !data) return;
    
    jsize len = env->GetArrayLength(data);
    if (len <= 0) return;
    
    std::vector<jbyte> buffer(len);
    env->GetByteArrayRegion(data, 0, len, buffer.data());
    
    h->frame_analyzer->feed(reinterpret_cast<const uint8_t*>(buffer.data()), len);
    h->protocol_analyzer->feedUsbPayload(reinterpret_cast<const uint8_t*>(buffer.data()), len, timestamp, host_to_device);
}

JNIEXPORT jobjectArray JNICALL
Java_com_yuhanbae_ca3_NativeCa3_nativeFindCandidates(JNIEnv* env, jobject thiz, jlong handle) {
    NativeHandle* h = getHandle(handle);
    if (!h || !h->frame_analyzer) return nullptr;
    
    auto candidates = h->frame_analyzer->findCandidates();
    
    jclass candidateClass = env->FindClass("com/yuhanbae/ca3/FrameCandidate");
    if (!candidateClass) return nullptr;
    
    jmethodID constructor = env->GetMethodID(candidateClass, "<init>", "(IIIIIZLjava/lang/String;DLjava/lang/String;)V");
    if (!constructor) return nullptr;
    
    jobjectArray result = env->NewObjectArray(candidates.size(), candidateClass, nullptr);
    
    for (size_t i = 0; i < candidates.size(); ++i) {
        const auto& c = candidates[i];
        jobject obj = env->NewObject(candidateClass, constructor,
            c.offset, c.length, c.possible_start_byte, c.possible_length_byte,
            c.has_valid_crc,
            env->NewStringUTF(c.crc_type.c_str()),
            c.confidence,
            env->NewStringUTF(c.hypothesis.c_str())
        );
        env->SetObjectArrayElement(result, i, obj);
    }
    
    return result;
}

JNIEXPORT jobject JNICALL
Java_com_yuhanbae_ca3_NativeCa3_nativeAnalyzeFrame(JNIEnv* env, jobject thiz, jlong handle, jbyteArray data) {
    NativeHandle* h = getHandle(handle);
    if (!h || !h->frame_analyzer || !data) return nullptr;
    
    jsize len = env->GetArrayLength(data);
    if (len <= 0) return nullptr;
    
    std::vector<jbyte> buffer(len);
    env->GetByteArrayRegion(data, 0, len, buffer.data());
    
    auto analysis = h->frame_analyzer->analyzeFrame(reinterpret_cast<const uint8_t*>(buffer.data()), len);
    
    jclass analysisClass = env->FindClass("com/yuhanbae/ca3/FrameAnalysis");
    if (!analysisClass) return nullptr;
    
    jmethodID constructor = env->GetMethodID(analysisClass, "<init>", "([B[Ljava/lang/String;IILjava/lang/String;)V");
    if (!constructor) return nullptr;
    
    // Payload
    jbyteArray payloadArray = env->NewByteArray(analysis.payload.size());
    env->SetByteArrayRegion(payloadArray, 0, analysis.payload.size(), reinterpret_cast<const jbyte*>(analysis.payload.data()));
    
    // Hypotheses
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray hypothesesArray = env->NewObjectArray(analysis.hypotheses.size(), stringClass, nullptr);
    for (size_t i = 0; i < analysis.hypotheses.size(); ++i) {
        env->SetObjectArrayElement(hypothesesArray, i, env->NewStringUTF(analysis.hypotheses[i].c_str()));
    }
    
    jobject obj = env->NewObject(analysisClass, constructor,
        payloadArray,
        hypothesesArray,
        analysis.length_field_pos.value_or(-1),
        analysis.cmd_field_pos.value_or(-1),
        analysis.crc_info ? env->NewStringUTF(analysis.crc_info->second.c_str()) : nullptr
    );
    
    return obj;
}

JNIEXPORT jobjectArray JNICALL
Java_com_yuhanbae_ca3_NativeCa3_nativeGetJ1939Frames(JNIEnv* env, jobject thiz, jlong handle) {
    NativeHandle* h = getHandle(handle);
    if (!h || !h->protocol_analyzer) return nullptr;
    
    const auto& frames = h->protocol_analyzer->getJ1939Frames();
    
    jclass frameClass = env->FindClass("com/yuhanbae/ca3/J1939Frame");
    if (!frameClass) return nullptr;
    
    jmethodID constructor = env->GetMethodID(frameClass, "<init>", "(I[BBIJ)V");
    if (!constructor) return nullptr;
    
    jobjectArray result = env->NewObjectArray(frames.size(), frameClass, nullptr);
    
    for (size_t i = 0; i < frames.size(); ++i) {
        const auto& f = frames[i];
        jbyteArray dataArray = env->NewByteArray(8);
        env->SetByteArrayRegion(dataArray, 0, 8, reinterpret_cast<const jbyte*>(f.data));
        
        jobject obj = env->NewObject(frameClass, constructor,
            f.can_id,
            dataArray,
            f.dlc,
            f.getPGN(),
            f.timestamp_ns
        );
        env->SetObjectArrayElement(result, i, obj);
    }
    
    return result;
}

JNIEXPORT jobjectArray JNICALL
Java_com_yuhanbae_ca3_NativeCa3_nativeGetHypotheses(JNIEnv* env, jobject thiz, jlong handle) {
    NativeHandle* h = getHandle(handle);
    if (!h || !h->protocol_analyzer) return nullptr;
    
    const auto& hypotheses = h->protocol_analyzer->getHypotheses();
    
    jclass hypClass = env->FindClass("com/yuhanbae/ca3/ProtocolHypothesis");
    if (!hypClass) return nullptr;
    
    jmethodID constructor = env->GetMethodID(hypClass, "<init>", "(ILjava/lang/String;DLjava/lang/String;Ljava/lang/String;)V");
    if (!constructor) return nullptr;
    
    jobjectArray result = env->NewObjectArray(hypotheses.size(), hypClass, nullptr);
    
    for (size_t i = 0; i < hypotheses.size(); ++i) {
        const auto& h = hypotheses[i];
        jbyteArray sample = nullptr;
        if (!h.sample_frame.empty()) {
            sample = env->NewByteArray(h.sample_frame.size());
            env->SetByteArrayRegion(sample, 0, h.sample_frame.size(), reinterpret_cast<const jbyte*>(h.sample_frame.data()));
        }
        
        jobject obj = env->NewObject(hypClass, constructor,
            static_cast<jint>(h.type),
            env->NewStringUTF(h.description.c_str()),
            h.confidence,
            env->NewStringUTF(h.notes.c_str()),
            env->NewStringUTF(h.sample_frame.empty() ? "" : "")
        );
        env->SetObjectArrayElement(result, i, obj);
    }
    
    return result;
}

JNIEXPORT void JNICALL
Java_com_yuhanbae_ca3_NativeCa3_nativeClear(JNIEnv* env, jobject thiz, jlong handle) {
    NativeHandle* h = getHandle(handle);
    if (!h) return;
    
    h->frame_analyzer->clear();
    h->protocol_analyzer->clear();
}

JNIEXPORT jobject JNICALL
Java_com_yuhanbae_ca3_NativeCa3_nativeGetStats(JNIEnv* env, jobject thiz, jlong handle) {
    NativeHandle* h = getHandle(handle);
    if (!h || !h->protocol_analyzer) return nullptr;
    
    auto stats = h->protocol_analyzer->getStats();
    
    jclass statsClass = env->FindClass("com/yuhanbae/ca3/ProtocolStats");
    if (!statsClass) return nullptr;
    
    jmethodID constructor = env->GetMethodID(statsClass, "<init>", "(JJJJ)V");
    if (!constructor) return nullptr;
    
    return env->NewObject(statsClass, constructor,
        stats.usb_packets_received,
        stats.usb_packets_sent,
        stats.j1939_frames_decoded,
        stats.iso_tp_frames_decoded
    );
}

// CRC testing
JNIEXPORT jobjectArray JNICALL
Java_com_yuhanbae_ca3_NativeCa3_nativeTestCrc(JNIEnv* env, jobject thiz, jlong handle, jbyteArray data, jlong expected_crc) {
    NativeHandle* h = getHandle(handle);
    if (!h || !data) return nullptr;
    
    jsize len = env->GetArrayLength(data);
    if (len <= 0) return nullptr;
    
    std::vector<jbyte> buffer(len);
    env->GetByteArrayRegion(data, 0, len, buffer.data());
    
    auto results = ca3::CrcCalculator::testAll(reinterpret_cast<const uint8_t*>(buffer.data()), len, expected_crc);
    
    jclass resultClass = env->FindClass("com/yuhanbae/ca3/CrcResult");
    if (!resultClass) return nullptr;
    
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(Ljava/lang/String;JZ)V");
    if (!constructor) return nullptr;
    
    jobjectArray result = env->NewObjectArray(results.size(), resultClass, nullptr);
    
    for (size_t i = 0; i < results.size(); ++i) {
        jobject obj = env->NewObject(resultClass, constructor,
            env->NewStringUTF(results[i].name.c_str()),
            results[i].value,
            results[i].matches
        );
        env->SetObjectArrayElement(result, i, obj);
    }
    
    return result;
}

} // extern "C"