// Copyright (c) 2014, Facebook, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
#pragma once

#include <functional>
#include <memory>
#include "src/messages/queues.h"
#include "src/util/common/flow_control.h"

namespace rocketspeed {

class Logger;
class QueueStats;

/**
 * Creates a queue and registers a callback on a flow control object.
 *
 * @param info_log Logging for queue.
 * @param queue_stats Statistics for queue.
 * @param size Size of the queue (in elements).
 * @param flow_control Flow control to use for processing the queue elements.
 * @param callback Callback to invoke on queue reads.
 */
template <typename T>
std::shared_ptr<Queue<T>>
InstallQueue(std::shared_ptr<Logger> info_log,
             std::shared_ptr<QueueStats> queue_stats,
             size_t size,
             FlowControl* flow_control,
             std::function<void(Flow*, T)> callback) {
  auto queue = std::make_shared<Queue<T>>(std::move(info_log),
                                          std::move(queue_stats),
                                          size);
  flow_control->Register<T>(queue.get(), std::move(callback));
  return queue;
}

}  // namespace rocketspeed
