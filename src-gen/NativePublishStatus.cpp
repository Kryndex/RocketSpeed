// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from rocketspeed.djinni

#include "NativePublishStatus.hpp"  // my header
#include "NativeMsgIdImpl.hpp"
#include "NativeStatus.hpp"

namespace djinni_generated {

jobject NativePublishStatus::toJava(JNIEnv* jniEnv, ::rocketspeed::djinni::PublishStatus c) {
    djinni::LocalRef<jobject> j_status(jniEnv, NativeStatus::toJava(jniEnv, c.status));
    djinni::LocalRef<jobject> j_message_id(jniEnv, NativeMsgIdImpl::toJava(jniEnv, c.message_id));
    const auto & data = djinni::JniClass<::djinni_generated::NativePublishStatus>::get();
    jobject r = jniEnv->NewObject(data.clazz.get(), data.jconstructor, j_status.get(), j_message_id.get());
    djinni::jniExceptionCheck(jniEnv);
    return r;
}

::rocketspeed::djinni::PublishStatus NativePublishStatus::fromJava(JNIEnv* jniEnv, jobject j) {
    assert(j != nullptr);
    const auto & data = djinni::JniClass<::djinni_generated::NativePublishStatus>::get();
    return ::rocketspeed::djinni::PublishStatus(
        NativeStatus::fromJava(jniEnv, djinni::LocalRef<jobject>(jniEnv, jniEnv->GetObjectField(j, data.field_status)).get()),
        NativeMsgIdImpl::fromJava(jniEnv, djinni::LocalRef<jobject>(jniEnv, jniEnv->GetObjectField(j, data.field_messageId)).get()));
}

}  // namespace djinni_generated
