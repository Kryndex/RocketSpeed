// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from rocketspeed.djinni

#pragma once

#include "Status.hpp"
#include <cstdint>

namespace rocketspeed { namespace djinni {

class SubscribeCallbackImpl {
public:
    virtual ~SubscribeCallbackImpl() {}

    virtual void Call(Status status, int64_t sequence_number, bool subscribed) = 0;
};

} }  // namespace rocketspeed::djinni