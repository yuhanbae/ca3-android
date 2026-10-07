#include <jni.h>
#include <android/log.h>

#include <vector>
#include <mutex>

#define TAG "CA3JNI"

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

struct UsbState {
    jobject connection = nullptr;
    jint interfaceNumber = -1;

    jint epIn = -1;
    jint epOut = -1;

    jclass connectionClass = nullptr;

    jmethodID bulkTransfer = nullptr;
};

static UsbState* state(
    jlong ptr
) {
    return reinterpret_cast<UsbState*>(ptr);
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeCreate(
    JNIEnv* env,
    jobject /* thiz */
) {
    auto* s = new UsbState();

    LOGI("nativeCreate");

    return reinterpret_cast<jlong>(s);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeDestroy(
    JNIEnv* env,
    jobject /* thiz */,
    jlong ptr
) {
    auto* s = state(ptr);

    if (!s)
        return;

    LOGI("nativeDestroy");

    if (s->connection) {
        env->DeleteGlobalRef(s->connection);
    }

    if (s->connectionClass) {
        env->DeleteGlobalRef(s->connectionClass);
    }

    delete s;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeAttachConnection(
    JNIEnv* env,
    jobject /* thiz */,
    jlong ptr,
    jobject connection
) {
    auto* s = state(ptr);

    if (!s || !connection)
        return JNI_FALSE;

    if (s->connection) {
        env->DeleteGlobalRef(s->connection);
    }

    s->connection =
        env->NewGlobalRef(connection);

    if (!s->connection) {
        LOGE("NewGlobalRef(connection) failed");
        return JNI_FALSE;
    }

    jclass localClass =
        env->GetObjectClass(connection);

    if (!localClass) {
        LOGE("GetObjectClass failed");
        return JNI_FALSE;
    }

    s->connectionClass =
        reinterpret_cast<jclass>(
            env->NewGlobalRef(localClass)
        );

    env->DeleteLocalRef(localClass);

    if (!s->connectionClass) {
        LOGE("NewGlobalRef(class) failed");
        return JNI_FALSE;
    }

    s->bulkTransfer =
        env->GetMethodID(
            s->connectionClass,
            "bulkTransfer",
            "(Landroid/hardware/usb/UsbEndpoint;[BIII)I"
        );

    if (!s->bulkTransfer) {
        LOGE("bulkTransfer method not found");
        return JNI_FALSE;
    }

    LOGI("UsbDeviceConnection attached");

    return JNI_TRUE;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeBulkTransfer(
    JNIEnv* env,
    jobject /* thiz */,
    jlong ptr,
    jobject endpoint,
    jbyteArray data,
    jint offset,
    jint length,
    jint timeout
) {
    auto* s = state(ptr);

    if (!s || !s->connection)
        return -1;

    if (!endpoint || !data)
        return -2;

    if (offset < 0 || length <= 0)
        return -3;

    jsize arrayLength =
        env->GetArrayLength(data);

    if (offset + length > arrayLength)
        return -4;

    return env->CallIntMethod(
        s->connection,
        s->bulkTransfer,
        endpoint,
        data,
        offset,
        length,
        timeout
    );
}