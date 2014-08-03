// Copyright (c) 2014, Facebook, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
#pragma once

#include <inttypes.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/thread.h>
#include <event2/util.h>
#include <functional>
#include <memory>
#include "include/Env.h"
#include "src/messages/serializer.h"
#include "src/messages/messages.h"
#include "src/util/logging.h"
#include "src/util/log_buffer.h"
#include "src/util/auto_roll_logger.h"

namespace rocketspeed {

typedef void* EventCallbackContext;
typedef std::function<void(EventCallbackContext, std::unique_ptr<Message> msg)>
                                            EventCallbackType;

class EventLoop {
 public:
  /*
   * Create an EventLoop at the specified port.
   * @param port The port on which the EventLoop is running
   * @param info_log Write informational messages to this log
   * @param callback The callback method that is invoked for every msg received
   */
  EventLoop(int port,
            const std::shared_ptr<Logger>& info_log,
            EventCallbackType callback);

  virtual ~EventLoop();

  /**
   *  Set the callback context
   * @param arg A opaque blob that is passed back to every invocation of
   *            event_callback_.
   */
  void SetEventCallbackContext(EventCallbackContext ctx) {
    event_callback_context_ = ctx;
  }

  // Start this instance of the Event Loop
  void Run(void);

  // Is the EventLoop up and running?
  bool IsRunning() { return running_; }

 private:
  // the port nuber of
  int port_number_;

  // Is the EventLoop all setup and running?
  bool running_;

  // The event loop base.
  struct event_base *base_;

  // debug message go here
  const std::shared_ptr<Logger> info_log_;

  // The callback
  EventCallbackType event_callback_;

  // The callback context
  EventCallbackContext event_callback_context_;

  // callbacks needed by libevent
  static void readhdr(struct bufferevent *bev, void *ctx);
  static void readmsg(struct bufferevent *bev, void *ctx);
  static void errorcb(struct bufferevent *bev, short error, void *ctx);
  static void do_accept(evutil_socket_t listener, short event, void *arg);
  static void do_startevent(evutil_socket_t listener, short event, void *arg);
  static void dump_libevent_cb(int severity, const char* msg);
};

}  // namespace rocketspeed
