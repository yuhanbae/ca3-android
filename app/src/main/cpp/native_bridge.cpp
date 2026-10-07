#include <jni.h>
#include <android/log.h>
#include <memory>
#include <vector>
#include <string>

#define LOG_TAG "CA3Native"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

struct NativeHandle {
    int dummy = 0;
};

static NativeHandle* getHandle(jlong handle) {
    return reinterpret_cast<NativeHandle*>(handle);
}

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeCreate(JNIEnv* env, jobject thiz) {
    auto handle = new NativeHandle();
    LOGI("Native handle created: %p", handle);
    return reinterpret_cast<jlong>(handle);
}

JNIEXPORT void JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeDestroy(JNIEnv* env, jobject thiz, jlong handle) {
    NativeHandle* h = getHandle(handle);
    if (h) {
        delete h;
        LOGI("Native handle destroyed");
    }
}

JNIEXPORT jboolean JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeInitialize(JNIEnv* env, jobject thiz, jlong handle) {
    NativeHandle* h = getHandle(handle);
    if (!h) return JNI_FALSE;
    LOGI("Native initialize called");
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeUsbOpen(JNIEnv* env, jobject thiz, jlong handle, jint vid, jint pid) {
    NativeHandle* h = getHandle(handle);
    if (!h) return JNI_FALSE;
    LOGI("Native USB open: VID=0x%04X PID=0x%04X", vid, pid);
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeUsbClose(JNIEnv* env, jobject thiz, jlong handle) {
    NativeHandle* h = getHandle(handle);
    if (!h) return;
    LOGI("Native USB close");
}

JNIEXPORT jint JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeUsbRead(JNIEnv* env, jobject thiz, jlong handle, jbyteArray buffer, jint offset, jint length, jint timeout) {
    NativeHandle* h = getHandle(handle);
    if (!h) return -1;

    jbyte* buf = env->GetByteArrayElements(buffer, nullptr);
    int result = 0; // Placeholder - actual USB read would go here
    env->ReleaseByteArrayElements(buffer, buf, 0);
    return result;
}

JNIEXPORT jint JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeUsbWrite(JNIEnv* env, jobject thiz, jlong handle, jbyteArray buffer, jint offset, jint length, jint timeout) {
    NativeHandle* h = getHandle(handle);
    if (!h) return -1;

    jbyte* buf = env->GetByteArrayElements(buffer, nullptr);
    int result = 0; // Placeholder - actual USB write would go here
    env->ReleaseByteArrayElements(buffer, buf, 0);
    return result;
}

JNIEXPORT jint JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeControlTransfer(JNIEnv* env, jobject thiz, jlong handle, jint requestType, jint request, jint value, jint index, jbyteArray buffer, jint length, jint timeout) {
    NativeHandle* h = getHandle(handle);
    if (!h) return -1;

    jbyte* buf = env->GetByteArrayElements(buffer, nullptr);
    int result = 0; // Placeholder - actual control transfer would go here
    env->ReleaseByteArrayElements(buffer, buf, 0);
    return result;
}

JNIEXPORT jstring JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeGetVersion(JNIEnv* env, jobject thiz, jlong handle) {
    return env->NewStringUTF("CA3Bridge Native v1.0.0");
}

JNIEXPORT jobjectArray JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeGetSupportedProtocols(JNIEnv* env, jobject thiz, jlong handle) {
    const char* protocols[] = {"CAT", "J1939", "PLUS1", "CAN"};
    int count = sizeof(protocols) / sizeof(protocols[0]);

    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray result = env->NewObjectArray(count, stringClass, nullptr);

    for (int i = 0; i < count; ++i) {
        env->SetObjectArrayElement(result, i, env->NewStringUTF(protocols[i]));
    }

    return result;
}

// CRC functions for protocol analysis
static uint16_t crc16_modbus(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
        }
    }
    return crc;
}

static uint32_t crc32_j1939(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    static const uint32_t table[256] = {0}; // Would be initialized
    for (size_t i = 0; i < len; ++i) {
        crc = (crc >> 8) ^ table[(crc ^ data[i]) & 0xFF];
    }
    return ~crc;
}

JNIEXPORT jint JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeCrc16Modbus(JNIEnv* env, jobject thiz, jlong handle, jbyteArray data, jint length) {
    NativeHandle* h = getHandle(handle);
    if (!h || !data) return 0;

    jbyte* buf = env->GetByteArrayElements(data, nullptr);
    uint16_t crc = crc16_modbus(reinterpret_cast<const uint8_t*>(buf), length);
    env->ReleaseByteArrayElements(data, buf, JNI_ABORT);
    return static_cast<jint>(crc);
}

JNIEXPORT jint JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeCrc32J1939(JNIEnv* env, jobject thiz, jlong handle, jbyteArray data, jint length) {
    NativeHandle* h = getHandle(handle);
    if (!h || !data) return 0;

    jbyte* buf = env->GetByteArrayElements(data, nullptr);
    uint32_t crc = crc32_j1939(reinterpret_cast<const uint8_t*>(buf), length);
    env->ReleaseByteArrayElements(data, buf, JNI_ABORT);
    return static_cast<jint>(crc);
}

} // extern "C"