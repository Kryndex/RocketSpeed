// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from rocketspeed.djinni

#include "NativeTopicOptions.hpp"  // my header
#include "NativeRetention.hpp"

namespace djinni_generated {

jobject NativeTopicOptions::toJava(JNIEnv* jniEnv, ::rocketglue::TopicOptions c) {
    djinni::LocalRef<jobject> j_retention(jniEnv, NativeRetention::toJava(jniEnv, c.retention));
    const auto & data = djinni::JniClass<::djinni_generated::NativeTopicOptions>::get();
    jobject r = jniEnv->NewObject(data.clazz.get(), data.jconstructor, j_retention.get());
    djinni::jniExceptionCheck(jniEnv);
    return r;
}

::rocketglue::TopicOptions NativeTopicOptions::fromJava(JNIEnv* jniEnv, jobject j) {
    assert(j != nullptr);
    const auto & data = djinni::JniClass<::djinni_generated::NativeTopicOptions>::get();
    return ::rocketglue::TopicOptions(
        NativeRetention::fromJava(jniEnv, djinni::LocalRef<jobject>(jniEnv, jniEnv->GetObjectField(j, data.field_mRetention)).get()));
}

}  // namespace djinni_generated