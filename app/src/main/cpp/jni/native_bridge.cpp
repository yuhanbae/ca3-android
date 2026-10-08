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

extern "C"
JNIEXPORT jint JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeSetInterface(
    JNIEnv* env,
    jobject /* thiz */,
    jlong ptr,
    jint interfaceNumber
) {
    auto* s = state(ptr);
    if (!s) return -1;
    s->interfaceNumber = interfaceNumber;
    LOGI("nativeSetInterface %d", interfaceNumber);
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeSetEndpoints(
    JNIEnv* env,
    jobject /* thiz */,
    jlong ptr,
    jint epIn,
    jint epOut
) {
    auto* s = state(ptr);
    if (!s) return -1;
    s->epIn = epIn;
    s->epOut = epOut;
    LOGI("nativeSetEndpoints in=%d out=%d", epIn, epOut);
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeGetInfo(
    JNIEnv* env,
    jobject /* thiz */,
    jlong ptr,
    jobject infoObj
) {
    auto* s = state(ptr);
    if (!s || !infoObj) return -1;
    LOGI("nativeGetInfo");
    // Populate ca3_usb_info_t Kotlin data class fields from internal state
    jclass infoClass = env->GetObjectClass(infoObj);
    if (!infoClass) {
        LOGE("GetObjectClass for infoObj failed");
        return -1;
    }
    auto setIntField = [&](const char* name) -> jfieldID {
        jfieldID fid = env->GetFieldID(infoClass, name, "I");
        if (!fid) {
            LOGE("Field %s not found", name);
        }
        return fid;
    };
    // interface_number, ep_in, ep_out
    jfieldID fidInterface = setIntField("interface_number");
    jfieldID fidEpIn = setIntField("ep_in");
    jfieldID fidEpOut = setIntField("ep_out");
    if (fidInterface) env->SetIntField(infoObj, fidInterface, s->interfaceNumber);
    if (fidEpIn) env->SetIntField(infoObj, fidEpIn, s->epIn);
    if (fidEpOut) env->SetIntField(infoObj, fidEpOut, s->epOut);
    // Optionally fill vid/pid with zeros - could be extended to query UsbDevice
    jfieldID fidVid = setIntField("vid");
    jfieldID fidPid = setIntField("pid");
    if (fidVid) env->SetIntField(infoObj, fidVid, 0);
    if (fidPid) env->SetIntField(infoObj, fidPid, 0);
    env->DeleteLocalRef(infoClass);
    return 0;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_fieldtools_ca3bridge_NativeBridge_nativeLastError(
    JNIEnv* env,
    jobject /* thiz */,
    jlong ptr
) {
    auto* s = state(ptr);
    if (!s) {
        return env->NewStringUTF("invalid handle");
    }
    LOGI("nativeLastError");
    // No persistent error state yet; return empty string to indicate no error
    return env->NewStringUTF("");
}
