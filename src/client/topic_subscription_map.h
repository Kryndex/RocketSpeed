// Copyright (c) 2016, Facebook, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "include/Types.h"

namespace rocketspeed {

class SubscriptionState;
using SubscriptionID = uint64_t;

class TopicToSubscriptionMap {
 public:
  explicit TopicToSubscriptionMap(
      std::function<SubscriptionState*(SubscriptionID)> get_state);

  std::tuple<SubscriptionID, SubscriptionState*> Find(
      const NamespaceID& namespace_id, const Topic& topic_name) const;

  void Insert(const NamespaceID& namespace_id,
              const Topic& topic_name,
              SubscriptionID sub_id);

  bool Remove(const NamespaceID& namespace_id,
              const Topic& topic_name,
              SubscriptionID sub_id);

 private:
  /**
   * Returns a SubscriptionState pointer for given ID or a null if the
   * subscription with the ID doesn't exist.
   */
  const std::function<SubscriptionState*(SubscriptionID)> get_state_;
  /**
   * A linear hashing scheme to map namespace and topic to the ID of the only
   * upstream subscription.
   */
  std::vector<SubscriptionID> vector_;
  /**
   * Cached allowed range of the number of upstream subscriptions for current
   * size of the open hashing data structure.
   */
  size_t sub_count_low_;
  size_t sub_count_high_;
  /**
   * Number of elements in the upstream_subscriptions_ vector. This might
   * diverge from the number of subscriptions known by the underlying subscriber
   * on certain occasion, and would be tricky to be kept the same at akk times.
   */
  size_t sub_count_;

  void InsertInternal(const NamespaceID& namespace_id,
                      const Topic& topic_name,
                      SubscriptionID sub_id);

  size_t FindOptimalPosition(const NamespaceID& namespace_id,
                             const Topic& topic_name) const;

  void Rehash();

  bool NeedsRehash() const;
};

}  // namespace rocketspeed