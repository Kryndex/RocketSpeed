// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from rocketspeed.djinni

#pragma once

#include "Status.hpp"

namespace rocketspeed { namespace djinni {

class SnapshotCallbackImpl {
public:
    virtual ~SnapshotCallbackImpl() {}

    virtual void Call(Status status) = 0;
};

} }  // namespace rocketspeed::djinni