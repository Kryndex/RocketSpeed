//  Copyright (c) 2014, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
//
#define __STDC_FORMAT_MACROS
#include "stream.h"

#include <cassert>
#include <memory>

#include "src/messages/event_loop.h"
#include "src/messages/messages.h"
#include "src/messages/serializer.h"
#include "src/messages/socket_event.h"

namespace rocketspeed {

Stream::Stream(SocketEvent* socket_event, StreamID remote_id, StreamID local_id)
: socket_event_(socket_event)
, remote_id_(remote_id)
, local_id_(local_id)
, receiver_(nullptr) {
  assert(socket_event_);
  thread_check_.Check();

  LOG_INFO(socket_event_->GetLogger(),
           "Created Stream(%llu, %llu)%s%s",
           local_id_,
           remote_id_,
           socket_event_->IsInbound() ? "" : " to: ",
           socket_event->GetDestination().ToString().c_str());
}

Stream::~Stream() {
  thread_check_.Check();

  if (socket_event_) {
    // Send a goodbye message to self to trigger stream closure.
    MessageGoodbye goodbye(Tenant::GuestTenant,
                           MessageGoodbye::Code::Graceful,
                           // Since we're sending a message the roles of server
                           // and client are exchanged.
                           socket_event_->IsInbound()
                               ? MessageGoodbye::OriginType::Server
                               : MessageGoodbye::OriginType::Client);
    Write(goodbye, true);
  }
  assert(!socket_event_);
}

bool Stream::Write(Message& message, bool check_thread) {
  assert(check_thread);
  thread_check_.Check();

  // Serialise the message.
  std::string str;
  message.SerializeToString(&str);
  return Write(str, check_thread);
}

bool Stream::Write(std::string& value, bool check_thread) {
  assert(check_thread);
  thread_check_.Check();

  auto serialised = std::make_shared<TimestampedString>();
  serialised->issued_time =
      socket_event_->GetEventLoop()->GetEnv()->NowMicros();
  serialised->string = std::move(value);
  return Write(serialised, check_thread);
}

bool Stream::Write(SharedTimestampedString& value, bool check_thread) {
  assert(check_thread);
  thread_check_.Check();

  if (!socket_event_) {
    // The stream is closed, just blackhole the value.
    // This could happen, as the stream might be closed spontaneously.
    LOG_INFO(socket_event_->GetLogger(),
             "Dropped message on closed Stream(%llu, %llu)",
             local_id_,
             remote_id_);
    return true;
  }

  // Sneak-peak message type, we will handle MessageGoodbye differently.
  auto type = Message::ReadMessageType(value->string);
  assert(type != MessageType::NotInitialized);

  LOG_DEBUG(socket_event_->GetLogger(),
            "Writing %zd bytes to Stream(%llu, %llu)",
            value->string.size(),
            local_id_,
            remote_id_);
  // Instead of associating a buffer with each stream, we use the one in the
  // socket.
  SerializedOnStream serialised;
  // When writing out the message we use the StreamID known by remote loop.
  serialised.stream_id = remote_id_;
  serialised.serialised = std::move(value);
  // Write a Goodbye message to the Socket, after SocketEvent::Write completes
  // the message is owned by the SocketEvent.
  const bool has_room = socket_event_->Write(serialised, check_thread);

  if (type == MessageType::mGoodbye) {
    // After sending a goodbye we must close the stream.
    LOG_INFO(socket_event_->GetLogger(),
             "Closing Stream(%llu, %llu) on owner's request",
             local_id_,
             remote_id_);
    // Mark stream as closed.
    socket_event_ = nullptr;
  }
  return has_room;
}

bool Stream::FlushPending(bool thread_check) {
  assert(thread_check);
  thread_check_.Check();
  assert(false);
  return true;
}

std::unique_ptr<EventCallback> Stream::CreateWriteCallback(
    EventLoop* event_loop, std::function<void()> callback) {
  thread_check_.Check();
  if (!socket_event_) {
    // The stream is closed, this shouldn't happen
    assert(false);
    return std::unique_ptr<EventCallback>();
  }
  // The stream is readable whenever the underlying socket is.
  return socket_event_->CreateWriteCallback(event_loop, std::move(callback));
}

void Stream::Receive(access::Stream,
                     Flow* flow,
                     std::unique_ptr<Message> message) {
  thread_check_.Check();

  // Abort if already closed.
  if (!socket_event_) {
    return;
  }

  if (message->GetMessageType() == MessageType::mGoodbye) {
    // Close the stream if this is a goodbye.
    auto goodbye = static_cast<const MessageGoodbye*>(message.get());
    LOG_INFO(socket_event_->GetLogger(),
             "Closing Stream(%llu, %llu), reason: %d",
             local_id_,
             remote_id_,
             static_cast<int>(goodbye->GetCode()));
    // Mark stream as closed.
    socket_event_ = nullptr;
  }

  if (receiver_) {
    StreamReceiveArg<Message> arg;
    arg.flow = flow;
    arg.stream_id = local_id_;
    arg.message = std::move(message);
    // We must not access any fields of this object after the callback is
    // invoked, as the callback may delete the stream object.
    (*receiver_)(std::move(arg));
  } else {
    LOG_DEBUG(socket_event_->GetLogger(),
              "Receiver not set for Stream(%llu, %llu), dropping message: %s",
              local_id_,
              remote_id_,
              MessageTypeName(message->GetMessageType()));
  }
}

}  // namespace rocketspeed