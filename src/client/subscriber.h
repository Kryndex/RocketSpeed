// Copyright (c) 2014, Facebook, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
//
#pragma once

#include <atomic>
#include <memory>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <tuple>

#include "include/Status.h"
#include "include/Types.h"
#include "include/RocketSpeed.h"
#include "include/SubscriptionStorage.h"
#include "include/BaseEnv.h"
#include "src/client/topic_subscription_map.h"
#include "src/messages/messages.h"
#include "src/messages/types.h"
#include "src/port/port.h"
#include "include/HostId.h"
#include "src/util/common/noncopyable.h"
#include "src/util/common/nonmovable.h"
#include "src/util/common/observable_set.h"
#include "src/util/common/random.h"
#include "src/util/common/ref_count_flyweight.h"
#include "src/util/common/statistics.h"
#include "src/util/timeout_list.h"

namespace rocketspeed {

class ClientOptions;
class Command;
class Flow;
class MessageDeliver;
class MessageUnsubscribe;
class MessageGoodbye;
class MsgLoop;
class EventCallback;
class EventLoop;
class Stream;
class SubscriptionState;
class SubscriberStats;
typedef uint64_t SubscriptionID;
typedef uint64_t SubscriptionHandle;
template <typename>
class ThreadLocalQueues;
template <typename>
class RateLimiterSink;

/**
 * An interface shared by all layers of subscribers.
 *
 * Common interface helps in proper unit testing of higher-level subscribers,
 * promotes separation of concerns and code reuse.
 */
class SubscriberIf {
 public:
  /** Destructor must close all remaining active subscriptions. */
  virtual ~SubscriberIf() = default;

  /**
   * Establishes a subscription with provided SubscriptionParameters.
   * Once the subscription is established, the application will be notified
   * about new data messages, gaps and termination of the subscription via
   * provided observer object.
   */
  virtual void StartSubscription(SubscriptionID sub_id,
                                 SubscriptionParameters parameters,
                                 std::unique_ptr<Observer> observer) = 0;

  /**
   * Marks provided message as acknowledged.
   * If SubscriptionStorage is being used, the Subscriber can resume
   * subscripions from storage starting from next unacknowledged message.
   */
  virtual void Acknowledge(SubscriptionID sub_id, SequenceNumber seqno) = 0;

  /** Terminates previously established subscription. */
  virtual void TerminateSubscription(SubscriptionID sub_id) = 0;

  /** True iff subscriber has no active subscriptions. */
  virtual bool Empty() const = 0;

  /** Saves state of the subscriber using provided storage strategy. */
  virtual Status SaveState(SubscriptionStorage::Snapshot* snapshot,
                           size_t worker_id) = 0;
};

struct TenantAndNamespace {
  TenantID tenant_id;
  NamespaceID namespace_id;
  friend bool operator<(const TenantAndNamespace& lhs,
                        const TenantAndNamespace& rhs) {
    if (lhs.tenant_id == rhs.tenant_id) {
      return lhs.namespace_id < rhs.namespace_id;
    }
    return lhs.tenant_id < rhs.tenant_id;
  }
};
typedef RefCountFlyweightFactory<TenantAndNamespace> TenantAndNamespaceFactory;
typedef RefCountFlyweight<TenantAndNamespace> TenantAndNamespaceFlyweight;

/**
 * Represents a state of a single subscription.
 */
class SubscriptionState : ThreadCheck, NonCopyable {
 public:
  // Movable
  SubscriptionState(SubscriptionState&&) = default;
  SubscriptionState& operator=(SubscriptionState&&) = default;

  SubscriptionState(const ThreadCheck& thread_check,
                    SubscriptionParameters parameters,
                    std::unique_ptr<Observer> observer,
                    TenantAndNamespaceFlyweight tenant_and_namespace)
  : ThreadCheck(thread_check)
  , observer_(std::move(observer))
  , tenant_and_namespace_(std::move(tenant_and_namespace))
  , topic_name_(std::move(parameters.topic_name))
  , expected_seqno_(parameters.start_seqno) {
    RS_ASSERT(!!observer_);
  }

  TenantID GetTenant() const { return tenant_and_namespace_.Get().tenant_id; }

  const NamespaceID& GetNamespace() const {
    return tenant_and_namespace_.Get().namespace_id;
  }

  const Topic& GetTopicName() const { return topic_name_; }

  void SwapObserver(std::unique_ptr<Observer>* observer) {
    std::swap(observer_, *observer);
  }

  Observer* GetObserver() { return observer_.get(); }

  /** Terminates subscription and notifies the application. */
  void Terminate(const std::shared_ptr<Logger>& info_log,
                 SubscriptionID sub_id,
                 MessageUnsubscribe::Reason reason);

  /** Processes gap or data message. */
  void ReceiveMessage(Flow* flow,
                      const std::shared_ptr<Logger>& info_log,
                      std::unique_ptr<MessageDeliver> deliver);

  /** Returns a lower bound on the seqno of the next expected message. */
  SequenceNumber GetExpected() const {
    ThreadCheck::Check();
    return expected_seqno_;
  }

 private:
  // Note: make sure the first data member has no common empty base classes
  //       with SubscriptionState. Otherwise compiler won't be able to perform
  //       Empty Base Optimization. For a more detailed explanation see:
  //       http://stackoverflow.com/a/547439
  std::unique_ptr<Observer> observer_;
  // Note: the following members are virtually const.
  //       That is, they should have been constant but they are not marked that
  //       because move-semantics wouldn't work for them in that case.
  //       Unfortunately, move semantics is currently not compatible with
  //       the concept of an immutable object in C++.
  /* const */ TenantAndNamespaceFlyweight tenant_and_namespace_;
  /* const */ Topic topic_name_;

  /** Next expected sequence number on this subscription. */
  SequenceNumber expected_seqno_;

  /** Returns true iff message arrived in order and not duplicated. */
  bool ProcessMessage(const std::shared_ptr<Logger>& info_log,
                      const MessageDeliver& deliver);
};

/**
 * A subscriber that manages subscription on a single shard.
 */
class Subscriber : public SubscriberIf, public StreamReceiver {
 public:
  Subscriber(const ClientOptions& options,
             EventLoop* event_loop,
             std::shared_ptr<SubscriberStats> stats,
             std::unique_ptr<SubscriptionRouter> router);

  ~Subscriber() override;

  void StartSubscription(SubscriptionID sub_id,
                         SubscriptionParameters parameters,
                         std::unique_ptr<Observer> observer) override;

  void Acknowledge(SubscriptionID sub_id, SequenceNumber seqno) override;

  void TerminateSubscription(SubscriptionID sub_id) override;

  bool Empty() const override { return subscriptions_.empty(); }

  Status SaveState(SubscriptionStorage::Snapshot* snapshot, size_t worker_id)
    override;

  SubscriptionState* GetState(SubscriptionID sub_id) {
    auto it = subscriptions_.find(sub_id);
    return it == subscriptions_.end() ? nullptr : &it->second;
  }

 private:
  ThreadCheck thread_check_;

  /** Options, whose lifetime must be managed by the owning client. */
  const ClientOptions& options_;
  /** An event loop object this subscriber runs on. */
  EventLoop* const event_loop_;
  /** A shared statistics. */
  std::shared_ptr<SubscriberStats> stats_;

  /** Time point (in us) until which client should not attempt to reconnect. */
  uint64_t backoff_until_time_;
  /** Time point (in us) of last message sending event. */
  uint64_t last_send_time_;
  /** Number of consecutive goodbye messages. */
  size_t consecutive_goodbyes_count_;
  /** Random engine used by this client. */
  std::mt19937_64& rng_;

  /** Stream socket used by this worker to talk to the Rocketeer. */
  std::unique_ptr<Stream> server_stream_;

  /** If subscription_rate_limit is set in ClientOptions,
   * the object holds stream decorator which applies rate limiting policy */
  std::unique_ptr<RateLimiterSink<SharedTimestampedString>>
      limited_server_stream_;

  /** The current server host. */
  HostId server_host_;

  /** Version of the router when we last fetched hosts. */
  size_t last_router_version_;
  /** The router for this subscriber. */
  std::unique_ptr<SubscriptionRouter> router_;

  /** Flyweight factory for tenant+namespace id pairs. */
  TenantAndNamespaceFactory tenant_and_namespace_factory_;

  /** All subscriptions served by this worker. */
  std::unordered_map<SubscriptionID, SubscriptionState> subscriptions_;

  /** All subscriptions served by this worker.
   *
   * If we were to restore state from subscription storage before the
   * subscription advances, we would restore from the next sequence number,
   * that is why we persist the previous one.
   */
  std::unordered_map<SubscriptionID, SequenceNumber> last_acks_map_;

  /** Start timer callback **/
  std::unique_ptr<EventCallback> start_timer_callback_;

  /**
   * A timeout list with recently sent unsubscribe requests, used to dedup
   * unsubscribes if we receive a burst of messages on terminated subscription.
   */
  TimeoutList<SubscriptionID> recent_terminations_;

  /**
   * A set of updated subscriptions (either new or terminated),
   * that has not been processed yet
   */
  ObservableSet<SubscriptionID> pending_subscriptions_;

  /**
   * Run periodic events.
   * It will reopen connection and initiate resubscription, if necessary.
   */
  void Tick();

  /** Try to reopen connection to copilot and reinitiate resubscriptions */
  void RestoreServerStream();
  /** Close connection to copilot and flush log of pending subscriptions */
  void CloseServerStream();
  /** Update and flush list of recently terminated subscripitons */
  void UpdateRecentTerminations();
  /** Check router config and reconnect if it was changed */
  void CheckRouterVersion();

  void ProcessPendingSubscription(
    Flow *flow, SubscriptionID sub_id, SubscriptionState* sub_state);

  void ReceiveDeliver(StreamReceiveArg<MessageDeliver> arg) final override;

  void ReceiveUnsubscribe(
      StreamReceiveArg<MessageUnsubscribe> arg) final override;

  void ReceiveGoodbye(StreamReceiveArg<MessageGoodbye> arg) final override;

  /** Write message to server stream using the provided flow object */
  void WriteToServerStream(Flow* flow, const Message& msg);

  /** Assert invariants, this is noop for release build */
  void CheckInvariants();

  /** Returns sequence number of last acknowledged message about
   * the given subscription id. */
  SequenceNumber GetLastAcknowledged(SubscriptionID sub_id) const;
};

namespace detail {
class TailCollapsingObserver;
}  // namespace detail

/**
 * A subscriber adaptor that collapses subscriptions, so that all downstream
 * subscriptions on a one particular topic are served from a single, tail
 * upstream subscription.
 */
class TailCollapsingSubscriber : public SubscriberIf {
 public:
  explicit TailCollapsingSubscriber(std::unique_ptr<Subscriber> subscriber);

  void StartSubscription(SubscriptionID sub_id,
                         SubscriptionParameters parameters,
                         std::unique_ptr<Observer> observer) override;

  void Acknowledge(SubscriptionID sub_id, SequenceNumber seqno) override;

  void TerminateSubscription(SubscriptionID sub_id) override;

  bool Empty() const override { return subscriber_->Empty(); }

  Status SaveState(SubscriptionStorage::Snapshot* snapshot, size_t worker_id)
    override;

 private:
  friend class detail::TailCollapsingObserver;

  ThreadCheck thread_check_;
  /** The underlying subscriber. */
  std::unique_ptr<Subscriber> subscriber_;

  /**
   * Maps ID of downstream subscription to the ID of the upstream one that
   * serves the former, only for collapsed subscriptions.
   */
  std::unordered_map<SubscriptionID, SubscriptionID> downstream_to_upstream_;
  /**
   * Stores all existing instances of TailCollapsingObserver created by this
   * subscriber. Useful when determining whether given upstream subscription
   * serves multiple downstream ones.
   */
  std::unordered_set<Observer*> special_observers_;
  /**
   * The map that we use for finding an upstream subscription for given topic
   * and namespace.
   */
  TopicToSubscriptionMap upstream_subscriptions_;
};

/**
 * A single threaded thread-unsafe subscriber, that lazily brings up subscribers
 * per shard.
 */
class alignas(CACHE_LINE_SIZE) MultiShardSubscriber : public SubscriberIf {
 public:
  MultiShardSubscriber(const ClientOptions& options,
                       EventLoop* event_loop,
                       std::shared_ptr<SubscriberStats> stats);

  ~MultiShardSubscriber() override;

  void StartSubscription(SubscriptionID sub_id,
                         SubscriptionParameters parameters,
                         std::unique_ptr<Observer> observer) override;

  void Acknowledge(SubscriptionID sub_id, SequenceNumber seqno) override;

  void TerminateSubscription(SubscriptionID sub_id) override;

  bool Empty() const override { return subscribers_.empty(); }

  Status SaveState(SubscriptionStorage::Snapshot* snapshot,
                   size_t worker_id) override;

 private:
  /** Options, whose lifetime must be managed by the owning client. */
  const ClientOptions& options_;
  /** An event loop object this subscriber runs on. */
  EventLoop* const event_loop_;

  /**
   * A map of subscribers one per each shard.
   * The map can be modified while some subscribers are running, therefore we
   * need them to be allocated separately.
   */
  std::unordered_map<size_t, std::unique_ptr<SubscriberIf>> subscribers_;

  /** A statistics object shared between subscribers. */
  std::shared_ptr<SubscriberStats> stats_;

  // TODO(t9432312)
  std::unordered_map<SubscriptionID, size_t> subscription_to_shard_;

  /**
   * Returns a subscriber for provided subscription ID or null if cannot
   * recognise the ID.
   */
  SubscriberIf* GetSubscriberForSubscription(SubscriptionID sub_id);
};

/** A multi-threaded subscriber. */
class MultiThreadedSubscriber {
 public:
  MultiThreadedSubscriber(const ClientOptions& options,
                          std::shared_ptr<MsgLoop> msg_loop);

  /**
   * Unsubscribes all subscriptions and prepares the subscriber for destruction.
   * Must be called while the MsgLoop this subscriber uses is still running.
   */
  void Stop();

  /**
   * Must be called after the MsgLoop this subscriber runs on is stopped.
   */
  ~MultiThreadedSubscriber();

  /**
   * If flow is non-null, the overflow is communicated via flow object.
   * Returns invalid SubscriptionHandle if and only if call attempt should be
   * retried due to queue overflow.
   */
  SubscriptionHandle Subscribe(Flow* flow,
                               SubscriptionParameters parameters,
                               std::unique_ptr<Observer> observer);

  /**
   * If flow is non-null, the overflow is communicated via flow object.
   * Returns false if and only if call attempt should be retried due to queue
   * overflow.
   */
  bool Unsubscribe(Flow* flow, SubscriptionHandle sub_handle);

  /**
   * If flow is non-null, the overflow is communicated via flow object.
   * Returns false if and only if call attempt should be retried due to queue
   * overflow.
   */
  bool Acknowledge(Flow* flow, const MessageReceived& message);

  // TODO(t9457879)
  void SaveSubscriptions(SaveSubscriptionsCallback save_callback);

  Statistics GetStatisticsSync();

 private:
  /** Options provided when creating the Client. */
  const ClientOptions& options_;
  /** A set of loops to use. */
  const std::shared_ptr<MsgLoop> msg_loop_;

  /** One multi-threaded subscriber per thread. */
  std::vector<std::unique_ptr<SubscriberIf>> subscribers_;
  /** Statistics per subscriber. */
  std::vector<std::shared_ptr<SubscriberStats>> statistics_;
  /** Queues to communicate with each subscriber. */
  std::vector<std::unique_ptr<ThreadLocalQueues<std::unique_ptr<Command>>>>
      subscriber_queues_;

  /** Next subscription ID seed to be used for new subscription ID. */
  std::atomic<uint64_t> next_sub_id_;

  /**
   * Returns a new subscription handle. This method is thread-safe.
   *
   * @param worker_id A worker this subscription will be bound to.
   * @return A handle, if fails to allocate returns a null-handle.
   */
  SubscriptionHandle CreateNewHandle(size_t worker_id);

  /**
   * Extracts worker ID from provided subscription handle.
   * In case of error, returned worker ID is negative.
   */
  ssize_t GetWorkerID(SubscriptionHandle sub_handle) const;
};

}  // namespace rocketspeed
