// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from rocketspeed.djinni

#pragma once

#include <memory>

namespace rocketglue {

class MessageReceived;

class MessageReceivedCallback {
public:
    virtual ~MessageReceivedCallback() {}

    virtual void Call(const std::shared_ptr<MessageReceived> & message_received) = 0;
};

}  // namespace rocketglue