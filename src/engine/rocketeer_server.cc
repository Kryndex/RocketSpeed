// Copyright (c) 2014, Facebook, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
//
#define __STDC_FORMAT_MACROS
#include "rocketeer_server.h"

#include <functional>
#include <memory>
#include <sstream>
#include <string>

#include "external/folly/move_wrapper.h"

#include "include/Logger.h"
#include "include/RocketSpeed.h"
#include "include/Slice.h"
#include "include/Status.h"
#include "include/Types.h"
#include "src/messages/msg_loop.h"
#include "src/port/Env.h"
#include "src/util/common/guid_generator.h"

namespace rocketspeed {

////////////////////////////////////////////////////////////////////////////////
RocketeerOptions::RocketeerOptions()
    : env(Env::Default())
    , info_log(std::make_shared<NullLogger>())
    , port(DEFAULT_PORT)
    , stats_prefix("rocketeer.") {
}

////////////////////////////////////////////////////////////////////////////////
class InboundSubscription {
 public:
  InboundSubscription(TenantID _tenant_id, SequenceNumber _prev_seqno)
      : tenant_id(_tenant_id), prev_seqno(_prev_seqno) {}

  TenantID tenant_id;
  SequenceNumber prev_seqno;
};

////////////////////////////////////////////////////////////////////////////////
class CommunicationRocketeer : public Rocketeer {
 public:
  explicit CommunicationRocketeer(Rocketeer* rocketeer);

  void HandleNewSubscription(InboundID inbound_id,
                             SubscriptionParameters params) final override;

  void HandleTermination(InboundID inbound_id,
                         Rocketeer::TerminationSource source) final override;

  void Deliver(InboundID inbound_id,
               SequenceNumber seqno,
               std::string payload,
               MsgId msg_id = MsgId()) final override;

  void Advance(InboundID inbound_id, SequenceNumber seqno) final override;

  void Terminate(InboundID inbound_id,
                 MessageUnsubscribe::Reason reason) final override;

  size_t GetID() const;

 private:
  friend class RocketeerServer;

  struct Stats {
    explicit Stats(const std::string& prefix);

    Counter* subscribes;
    Counter* unsubscribes;
    Counter* terminations;
    Counter* inbound_subscriptions;
    Counter* dropped_reordered;
    Statistics all;
  };

  ThreadCheck thread_check_;
  // RocketeerServer, which owns both the implementation and this object.
  RocketeerServer* server_;

  // The rocketeer that is being wrapped
  Rocketeer* above_rocketeer_;

  // An ID assigned by the server.
  size_t id_;
  std::unique_ptr<Stats> stats_;

  using SubscriptionsOnStream =
      std::unordered_map<SubscriptionID, InboundSubscription>;
  std::unordered_map<StreamID, SubscriptionsOnStream> inbound_subscriptions_;

  void Initialize(RocketeerServer* server, size_t id);

  const Statistics& GetStatisticsInternal();

  InboundSubscription* Find(const InboundID& inbound_id);

  void Receive(std::unique_ptr<MessageSubscribe> subscribe, StreamID origin);

  void Receive(std::unique_ptr<MessageUnsubscribe> unsubscribe,
               StreamID origin);

  void Receive(std::unique_ptr<MessageGoodbye> goodbye, StreamID origin);
};

CommunicationRocketeer::CommunicationRocketeer(Rocketeer* rocketeer)
: server_(nullptr), above_rocketeer_(rocketeer) {
  rocketeer->SetBelowRocketeer(this);
}

void CommunicationRocketeer::HandleNewSubscription(
    InboundID inbound_id, SubscriptionParameters params) {
  above_rocketeer_->HandleNewSubscription(inbound_id, std::move(params));
}

void CommunicationRocketeer::HandleTermination(InboundID inbound_id,
                                               TerminationSource source) {
  above_rocketeer_->HandleTermination(inbound_id, source);
}

void CommunicationRocketeer::Deliver(InboundID inbound_id,
                                     SequenceNumber seqno,
                                     std::string payload,
                                     MsgId msg_id) {
  thread_check_.Check();

  if (msg_id.Empty()) {
    msg_id = GUIDGenerator::ThreadLocalGUIDGenerator()->Generate();
  }
  if (auto* sub = Find(inbound_id)) {
    if (sub->prev_seqno < seqno) {
      MessageDeliverData data(
          sub->tenant_id, inbound_id.sub_id, msg_id, payload);
      data.SetSequenceNumbers(sub->prev_seqno, seqno);
      sub->prev_seqno = seqno;
      server_->msg_loop_->SendResponse(
          data, inbound_id.stream_id, inbound_id.worker_id);
    } else {
      stats_->dropped_reordered->Add(1);
      LOG_WARN(server_->options_.info_log,
               "Attempted to deliver data at %" PRIu64
               ", but subscription has previous seqno %" PRIu64,
               seqno,
               sub->prev_seqno);
    }
  }
}

void CommunicationRocketeer::Advance(InboundID inbound_id,
                                     SequenceNumber seqno) {
  thread_check_.Check();

  if (auto* sub = Find(inbound_id)) {
    if (sub->prev_seqno < seqno) {
      MessageDeliverGap gap(
          sub->tenant_id, inbound_id.sub_id, GapType::kBenign);
      gap.SetSequenceNumbers(sub->prev_seqno, seqno);
      sub->prev_seqno = seqno;
      server_->msg_loop_->SendResponse(
          gap, inbound_id.stream_id, inbound_id.worker_id);
    } else {
      stats_->dropped_reordered->Add(1);
      LOG_WARN(server_->options_.info_log,
               "Attempted to deliver gap at %" PRIu64
               ", but subscription has previous seqno %" PRIu64,
               seqno,
               sub->prev_seqno);
    }
  }
}

void CommunicationRocketeer::Terminate(InboundID inbound_id,
                                       MessageUnsubscribe::Reason reason) {
  thread_check_.Check();

  StreamID origin = inbound_id.stream_id;
  SubscriptionID sub_id = inbound_id.sub_id;
  auto it = inbound_subscriptions_.find(origin);
  if (it != inbound_subscriptions_.end()) {
    auto it1 = it->second.find(sub_id);
    if (it1 != it->second.end()) {
      TenantID tenant_id = it1->second.tenant_id;
      it->second.erase(it1);
      stats_->inbound_subscriptions->Add(-1);
      stats_->terminations->Add(1);
      HandleTermination(InboundID(origin, sub_id, GetID()),
                        TerminationSource::Rocketeer);

      MessageUnsubscribe unsubscribe(tenant_id, inbound_id.sub_id, reason);
      server_->msg_loop_->SendResponse(
          unsubscribe, inbound_id.stream_id, inbound_id.worker_id);
      return;
    }
  }
  LOG_WARN(server_->options_.info_log,
           "Missing subscription on stream: %llu, sub_id: %" PRIu64,
           origin,
           sub_id);
}

size_t CommunicationRocketeer::GetID() const {
  auto worker_id = server_->msg_loop_->GetThreadWorkerIndex();
  RS_ASSERT(static_cast<size_t>(worker_id) == id_);
  ((void)worker_id);
  return id_;
}

void CommunicationRocketeer::Initialize(RocketeerServer* server, size_t id) {
  RS_ASSERT(!server_);
  server_ = server;
  id_ = id;
  stats_.reset(new Stats(server->options_.stats_prefix));
}

const Statistics& CommunicationRocketeer::GetStatisticsInternal() {
  RS_ASSERT(stats_);
  return stats_->all;
}

InboundSubscription* CommunicationRocketeer::Find(const InboundID& inbound_id) {
  auto it = inbound_subscriptions_.find(inbound_id.stream_id);
  if (it != inbound_subscriptions_.end()) {
    auto it1 = it->second.find(inbound_id.sub_id);
    if (it1 != it->second.end()) {
      return &it1->second;
    }
  }
  LOG_WARN(server_->options_.info_log,
           "Missing subscription on stream (%llu) with ID (%" PRIu64 ")",
           inbound_id.stream_id,
           inbound_id.sub_id);
  return nullptr;
}

void CommunicationRocketeer::Receive(
    std::unique_ptr<MessageSubscribe> subscribe, StreamID origin) {
  thread_check_.Check();

  SubscriptionID sub_id = subscribe->GetSubID();
  SequenceNumber start_seqno = subscribe->GetStartSequenceNumber();
  auto result = inbound_subscriptions_[origin].emplace(
      sub_id,
      InboundSubscription(subscribe->GetTenantID(),
                          start_seqno == 0 ? start_seqno : start_seqno - 1));
  if (!result.second) {
    LOG_WARN(server_->options_.info_log,
             "Duplicated subscription stream: %llu, sub_id: %" PRIu64,
             origin,
             subscribe->GetSubID());
    return;
  }
  // TODO(stupaq) store subscription parameters in a message and move them out
  SubscriptionParameters params(subscribe->GetTenantID(),
                                subscribe->GetNamespace(),
                                subscribe->GetTopicName(),
                                subscribe->GetStartSequenceNumber());
  HandleNewSubscription(InboundID(origin, sub_id, GetID()), std::move(params));
  stats_->subscribes->Add(1);
  stats_->inbound_subscriptions->Add(1);
}

void CommunicationRocketeer::Receive(
    std::unique_ptr<MessageUnsubscribe> unsubscribe, StreamID origin) {
  thread_check_.Check();

  SubscriptionID sub_id = unsubscribe->GetSubID();
  auto it = inbound_subscriptions_.find(origin);
  if (it != inbound_subscriptions_.end()) {
    auto removed = it->second.erase(sub_id);
    if (removed > 0) {
      stats_->inbound_subscriptions->Add(-1);
      stats_->unsubscribes->Add(1);
      HandleTermination(InboundID(origin, sub_id, GetID()),
                        TerminationSource::Subscriber);
      if (it->second.empty()) {
        inbound_subscriptions_.erase(it);
      }
      return;
    }
  }
  LOG_WARN(server_->options_.info_log,
           "Missing subscription on stream: %llu, sub_id: %" PRIu64,
           origin,
           sub_id);
}

void CommunicationRocketeer::Receive(std::unique_ptr<MessageGoodbye> goodbye,
                                     StreamID origin) {
  thread_check_.Check();

  auto it = inbound_subscriptions_.find(origin);
  if (it == inbound_subscriptions_.end()) {
    LOG_WARN(server_->options_.info_log, "Missing stream: %llu", origin);
    return;
  }
  for (const auto& entry : it->second) {
    stats_->inbound_subscriptions->Add(-1);
    stats_->unsubscribes->Add(1);
    HandleTermination(InboundID(origin, entry.first, GetID()),
                      TerminationSource::Subscriber);
  }
  inbound_subscriptions_.erase(it);
}

////////////////////////////////////////////////////////////////////////////////
CommunicationRocketeer::Stats::Stats(const std::string& prefix) {
  subscribes = all.AddCounter(prefix + "subscribes");
  unsubscribes = all.AddCounter(prefix + "unsubscribes");
  terminations = all.AddCounter(prefix + "terminations");
  inbound_subscriptions = all.AddCounter(prefix + "inbound_subscriptions");
  dropped_reordered = all.AddCounter(prefix + "dropped_reordered");
}

////////////////////////////////////////////////////////////////////////////////
RocketeerServer::RocketeerServer(RocketeerOptions options)
: options_(std::move(options)) {}

RocketeerServer::~RocketeerServer() {
  // Stop threads before any Rocketeer is destroyed.
  Stop();
}

size_t RocketeerServer::Register(Rocketeer* rocketeer) {
  RS_ASSERT(!msg_loop_);
  RS_ASSERT(rocketeer);
  auto id = rocketeers_.size();
  std::unique_ptr<CommunicationRocketeer> com_rocketeer;
  com_rocketeer.reset(new CommunicationRocketeer(rocketeer));
  com_rocketeer->Initialize(this, id);
  rocketeers_.push_back(std::move(com_rocketeer));
  return id;
}

Status RocketeerServer::Start() {
  msg_loop_.reset(new MsgLoop(options_.env,
                              EnvOptions(),
                              options_.port,
                              static_cast<int>(rocketeers_.size()),
                              options_.info_log,
                              "rocketeer"));

  Status st = msg_loop_->Initialize();
  if (!st.ok()) {
    return st;
  }

  msg_loop_->RegisterCallbacks({
      {MessageType::mSubscribe, CreateCallback<MessageSubscribe>()},
      {MessageType::mUnsubscribe, CreateCallback<MessageUnsubscribe>()},
      {MessageType::mGoodbye, CreateCallback<MessageGoodbye>()},
  });

  msg_loop_thread_.reset(
      new MsgLoopThread(options_.env, msg_loop_.get(), "rocketeer"));
  return Status::OK();
}

void RocketeerServer::Stop() {
  msg_loop_thread_.reset();
}

bool RocketeerServer::Deliver(InboundID inbound_id,
                              SequenceNumber seqno,
                              std::string payload,
                              MsgId msg_id) {
  auto moved_payload = folly::makeMoveWrapper(std::move(payload));
  auto command = [this, inbound_id, seqno, moved_payload, msg_id]() mutable {
    rocketeers_[inbound_id.worker_id]->Deliver(
        inbound_id, seqno, moved_payload.move(), msg_id);
  };
  return msg_loop_->SendCommand(std::unique_ptr<Command>(
                                    MakeExecuteCommand(std::move(command))),
                                inbound_id.worker_id)
      .ok();
}

bool RocketeerServer::Advance(InboundID inbound_id, SequenceNumber seqno) {
  auto command = [this, inbound_id, seqno]() mutable {
    rocketeers_[inbound_id.worker_id]->Advance(inbound_id, seqno);
  };
  return msg_loop_->SendCommand(std::unique_ptr<Command>(
                                    MakeExecuteCommand(std::move(command))),
                                inbound_id.worker_id)
      .ok();
}

bool RocketeerServer::Terminate(InboundID inbound_id,
                                MessageUnsubscribe::Reason reason) {
  auto command = [this, inbound_id, reason]() mutable {
    rocketeers_[inbound_id.worker_id]->Terminate(inbound_id, reason);
  };
  return msg_loop_->SendCommand(std::unique_ptr<Command>(
                                    MakeExecuteCommand(std::move(command))),
                                inbound_id.worker_id)
      .ok();
}

Statistics RocketeerServer::GetStatisticsSync() {
  auto stats = msg_loop_->AggregateStatsSync(
      [this](int i) { return rocketeers_[i]->GetStatisticsInternal(); });
  stats.Aggregate(msg_loop_->GetStatisticsSync());
  return stats;
}

template <typename Msg>
MsgCallbackType RocketeerServer::CreateCallback() {
  return [this](Flow* flow, std::unique_ptr<Message> message, StreamID origin) {
    std::unique_ptr<Msg> casted(static_cast<Msg*>(message.release()));
    auto worker_id = msg_loop_->GetThreadWorkerIndex();
    rocketeers_[worker_id]->Receive(std::move(casted), origin);
  };
}

}  // namespace rocketspeed
