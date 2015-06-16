// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from rocketspeed.djinni

#pragma once

#include "djinni_support.hpp"
#include "src-gen/djinni/cpp/SubscriptionParameters.hpp"

namespace djinni_generated {

class NativeSubscriptionParameters final {
public:
    using CppType = ::rocketspeed::djinni::SubscriptionParameters;
    using JniType = jobject;

    using Boxed = NativeSubscriptionParameters;

    ~NativeSubscriptionParameters();

    static CppType toCpp(JNIEnv* jniEnv, JniType j);
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c);

private:
    NativeSubscriptionParameters();
    friend ::djinni::JniClass<NativeSubscriptionParameters>;

    const ::djinni::GlobalRef<jclass> clazz { ::djinni::jniFindClass("org/rocketspeed/SubscriptionParameters") };
    const jmethodID jconstructor { ::djinni::jniGetMethodID(clazz.get(), "<init>", "(ILjava/lang/String;Ljava/lang/String;J)V") };
    const jfieldID field_tenantId { ::djinni::jniGetFieldID(clazz.get(), "tenantId", "I") };
    const jfieldID field_namespaceId { ::djinni::jniGetFieldID(clazz.get(), "namespaceId", "Ljava/lang/String;") };
    const jfieldID field_topicName { ::djinni::jniGetFieldID(clazz.get(), "topicName", "Ljava/lang/String;") };
    const jfieldID field_startSeqno { ::djinni::jniGetFieldID(clazz.get(), "startSeqno", "J") };
};

}  // namespace djinni_generated