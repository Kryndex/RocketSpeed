// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from rocketspeed.djinni

#pragma once

#include "ResultStatus.hpp"

namespace rocketglue {

/**
 *
 *
 * Implemented in Java and ObjC and can be called from C++.
 */
class PublishCallback {
public:
    virtual ~PublishCallback() {}

    virtual void Call(const ResultStatus & result_status) = 0;
};

}  // namespace rocketglue