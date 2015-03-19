// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from rocketspeed.djinni

#pragma once

#include "djinni_support.hpp"
#include "src-gen/djinni/cpp/ReceiveCallbackImpl.hpp"

namespace djinni_generated {

class NativeReceiveCallbackImpl final : djinni::JniInterface<::rocketspeed::djinni::ReceiveCallbackImpl, NativeReceiveCallbackImpl> {
public:
    using CppType = std::shared_ptr<::rocketspeed::djinni::ReceiveCallbackImpl>;
    using JniType = jobject;

    static jobject toJava(JNIEnv* jniEnv, std::shared_ptr<::rocketspeed::djinni::ReceiveCallbackImpl> c) { return djinni::JniClass<::djinni_generated::NativeReceiveCallbackImpl>::get()._toJava(jniEnv, c); }
    static std::shared_ptr<::rocketspeed::djinni::ReceiveCallbackImpl> fromJava(JNIEnv* jniEnv, jobject j) { return djinni::JniClass<::djinni_generated::NativeReceiveCallbackImpl>::get()._fromJava(jniEnv, j); }

    const djinni::GlobalRef<jclass> clazz { djinni::jniFindClass("org/rocketspeed/ReceiveCallbackImpl") };
    const jmethodID method_Call { djinni::jniGetMethodID(clazz.get(), "Call", "(ILjava/lang/String;J[B)V") };

    class JavaProxy final : djinni::JavaProxyCacheEntry, public ::rocketspeed::djinni::ReceiveCallbackImpl {
    public:
        JavaProxy(jobject obj);
        virtual void Call(int32_t namespace_id, std::string topic_name, int64_t sequence_number, std::vector<uint8_t> contents) override;

    private:
        using djinni::JavaProxyCacheEntry::getGlobalRef;
        friend class djinni::JniInterface<::rocketspeed::djinni::ReceiveCallbackImpl, ::djinni_generated::NativeReceiveCallbackImpl>;
        friend class djinni::JavaProxyCache<JavaProxy>;
    };

private:
    NativeReceiveCallbackImpl();
    friend class djinni::JniClass<::djinni_generated::NativeReceiveCallbackImpl>;
};

}  // namespace djinni_generated