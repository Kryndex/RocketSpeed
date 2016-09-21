// Copyright (c) 2016, Facebook, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
//
#include "include/Centrifuge.h"
#include "include/Env.h"
#include "src/tools/centrifuge/centrifuge.h"
#include <gflags/gflags.h>
#include <thread>
#include <cstdlib>

namespace rocketspeed {

/** Flags used by the generic RunCentrifugeClient runner. */
DEFINE_string(mode, "", "Which behaviour to use. Options are "
  "subscribe-rapid,"
  "subscribe-unsubscribe-rapid,"
  "slow-consumer");

DEFINE_uint64(num_subscriptions, 1000000,
  "Number of times to subscribe");

DEFINE_uint64(receive_sleep_ms, 1000,
  "Milliseconds to sleep on receiving a message");


namespace {
/** Sets the client and generator for a specicific behavior's options. */
template <typename BehaviorOptions>
void SetupGeneralOptions(CentrifugeOptions& general_options,
                         BehaviorOptions& behavior_options) {
  auto st = Client::Create(std::move(general_options.client_options),
                           &behavior_options.client);
  if (!st.ok()) {
    CentrifugeFatal(st);
  }
  behavior_options.generator = std::move(general_options.generator);
}
}

int RunCentrifugeClient(CentrifugeOptions options, int argc, char** argv) {
  GFLAGS::ParseCommandLineFlags(&argc, &argv, true);

  auto env = Env::Default();
  Status st = env->StdErrLogger(&centrifuge_logger);
  if (!st.ok()) {
    return 1;
  }

  int result = 0;
  if (FLAGS_mode == "subscribe-rapid") {
    SubscribeRapidOptions opts;
    SetupGeneralOptions(options, opts);
    opts.num_subscriptions = FLAGS_num_subscriptions;
    result = SubscribeRapid(std::move(opts));
  } else if (FLAGS_mode == "subscribe-unsubscribe-rapid") {
    SubscribeUnsubscribeRapidOptions opts;
    SetupGeneralOptions(options, opts);
    opts.num_subscriptions = FLAGS_num_subscriptions;
    result = SubscribeUnsubscribeRapid(std::move(opts));
  } else if (FLAGS_mode == "slow-consumer") {
    SlowConsumerOptions opts;
    SetupGeneralOptions(options, opts);
    opts.num_subscriptions = FLAGS_num_subscriptions;
    opts.receive_sleep_time = std::chrono::milliseconds(FLAGS_receive_sleep_ms);
    result = SlowConsumer(std::move(opts));
  } else {
    CentrifugeFatal(Status::InvalidArgument("Unknown mode flag"));
    return 1;
  }

  if (!result) {
    fprintf(stderr, "Centrifuge completed successfully.\n");
    fflush(stderr);
  }
  return result;
}

SubscribeRapidOptions::SubscribeRapidOptions()
: num_subscriptions(10000000) {}

int SubscribeRapid(SubscribeRapidOptions options) {
  auto num_subscriptions = options.num_subscriptions;
  std::unique_ptr<CentrifugeSubscription> sub;
  while (num_subscriptions-- && (sub = options.generator->Next())) {
    auto start = std::chrono::steady_clock::now();
    while (!options.client->Subscribe(sub->params, sub->observer)) {
      // Wait for backpressure to be lifted.
      /* sleep override */
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      if (std::chrono::steady_clock::now() - start > std::chrono::seconds(10)) {
        CentrifugeFatal(Status::TimedOut("Unable to subscribe for 10 seconds"));
        return 1;
      }
    }
  }
  return 0;
}

SubscribeUnsubscribeRapidOptions::SubscribeUnsubscribeRapidOptions()
: num_subscriptions(1000000) {}

int SubscribeUnsubscribeRapid(SubscribeUnsubscribeRapidOptions options) {
  auto num_subscriptions = options.num_subscriptions;
  std::unique_ptr<CentrifugeSubscription> sub;
  while (num_subscriptions-- && (sub = options.generator->Next())) {
    auto start = std::chrono::steady_clock::now();
    SubscriptionHandle handle;
    while (!(handle = options.client->Subscribe(sub->params, sub->observer))) {
      // Wait for backpressure to be lifted.
      /* sleep override */
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      if (std::chrono::steady_clock::now() - start > std::chrono::seconds(10)) {
        CentrifugeFatal(Status::TimedOut("Unable to subscribe for 10 seconds"));
        return 1;
      }
    }
    options.client->Unsubscribe(handle);
  }
  return 0;
}

namespace {
// Transforms a SubscriptionGenerator to slow down message receipt.
class SlowConsumerGenerator : public SubscriptionGenerator {
 public:
  SlowConsumerGenerator(std::unique_ptr<SubscriptionGenerator> gen,
                        std::chrono::milliseconds receive_sleep_time)
  : gen_(std::move(gen))
  , receive_sleep_time_(receive_sleep_time) {}

  std::unique_ptr<CentrifugeSubscription> Next() override {
    auto sub = gen_->Next();
    if (sub) {
      auto obs = std::move(sub->observer);
      sub->observer = SlowConsumerObserver(std::move(obs), receive_sleep_time_);
    }
    return sub;
  }

 private:
  std::unique_ptr<SubscriptionGenerator> gen_;
  const std::chrono::milliseconds receive_sleep_time_;
};
}

SlowConsumerOptions::SlowConsumerOptions()
: receive_sleep_time(1000) {}

int SlowConsumer(SlowConsumerOptions options) {
  auto gen = std::move(options.generator);
  options.generator.reset(
    new SlowConsumerGenerator(std::move(gen), options.receive_sleep_time));
  return SubscribeRapid(std::move(options));
}

}  // namespace rocketspeed
