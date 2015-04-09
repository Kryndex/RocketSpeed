// Copyright (c) 2014, Facebook, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
//
#include "src/test/test_cluster.h"
#include <memory>
#include <utility>
#include <vector>
#include <stdio.h>

#include "src/logdevice/storage.h"
#include "src/logdevice/log_router.h"

#ifdef USE_LOGDEVICE
#include "logdevice/include/debug.h"
#include "logdevice/test/utils/IntegrationTestUtils.h"
#endif  // USE_LOGDEVICE

namespace rocketspeed {

struct LocalTestCluster::LogDevice {
#ifdef USE_LOGDEVICE
  // LogDevice cluster and client.
  std::unique_ptr<facebook::logdevice::IntegrationTestUtils::Cluster> cluster_;
  std::shared_ptr<facebook::logdevice::Client> client_;
#endif  // USE_LOGDEVICE
  std::shared_ptr<LogStorage> storage_;
  std::shared_ptr<LogRouter> log_router_;
};

LocalTestCluster::LocalTestCluster(std::shared_ptr<Logger> info_log,
                                   bool start_controltower,
                                   bool start_copilot,
                                   bool start_pilot,
                                   const std::string& storage_url,
                                   Env* env) {
  Options opts;
  opts.info_log = info_log;
  opts.start_controltower = start_controltower;
  opts.start_copilot = start_copilot;
  opts.start_pilot = start_pilot;
  opts.storage_url = storage_url;
  opts.env = env;
  Initialize(opts);
}

LocalTestCluster::LocalTestCluster(Options opts) {
  Initialize(opts);
}

void LocalTestCluster::Initialize(Options opts) {
  logdevice_.reset(new LogDevice);
  env_ = opts.env;
  info_log_ = opts.info_log;
  pilot_ = nullptr;
  copilot_ = nullptr;
  control_tower_ = nullptr;
  cockpit_thread_ = 0;
  control_tower_thread_ = 0;

  if (opts.start_copilot && !opts.start_controltower) {
    status_ = Status::InvalidArgument("Copilot needs ControlTower.");
    return;
  }
#ifdef USE_LOGDEVICE
#ifdef NDEBUG
  // Disable LogDevice info logging in release.
  facebook::logdevice::dbg::currentLevel =
      facebook::logdevice::dbg::Level::WARNING;
#endif  // NDEBUG
#endif  // USE_LOGDEVICE

  // Range of logs to use.
  std::pair<LogID, LogID> log_range;
  if (opts.single_log) {
    log_range = std::pair<LogID, LogID>(1, 1);
  } else {
#ifdef USE_LOGDEVICE
    if (opts.storage_url.empty()) {
      // Can only use one log as the logdevice test utils only supports that.
      // See T4894216
      log_range = std::pair<LogID, LogID>(1, 1);
    } else {
      log_range = std::pair<LogID, LogID>(1, 100000);
    }
#else
    // Something more substantial for the mock logdevice.
    log_range = std::pair<LogID, LogID>(1, 1000);
#endif  // USE_LOGDEVICE
  }

  LogDeviceStorage* storage = nullptr;
  if (opts.start_pilot || opts.start_controltower) {
#ifdef USE_LOGDEVICE
    if (opts.storage_url.empty()) {
      // Setup the local LogDevice cluster and create, client, and storage.
      logdevice_->cluster_ =
          facebook::logdevice::IntegrationTestUtils::ClusterFactory().create(3);
      logdevice_->client_ = logdevice_->cluster_->createClient();
      status_ = LogDeviceStorage::Create(logdevice_->client_,
                                         env_,
                                         &storage);
    } else {
      status_ = LogDeviceStorage::Create("rocketspeed.logdevice.primary",
                                         opts.storage_url,
                                         "",
                                         std::chrono::milliseconds(1000),
                                         16,
                                         env_,
                                         &storage);
    }
#else
    status_ = LogDeviceStorage::Create("",
                                       "",
                                       "",
                                       std::chrono::milliseconds(1000),
                                       16,
                                       env_,
                                       &storage);
#endif  // USE_LOGDEVICE

    if (!status_.ok() || !storage) {
      LOG_ERROR(info_log_, "Failed to create LogDeviceStorage.");
      return;
    }
    logdevice_->storage_.reset(storage);
    storage = nullptr;
  }
  logdevice_->log_router_ =
    std::make_shared<LogDeviceLogRouter>(log_range.first, log_range.second);

  // Tell rocketspeed to use this storage interface/router.
  opts.pilot.storage = logdevice_->storage_;
  opts.pilot.log_router = logdevice_->log_router_;
  opts.copilot.log_router = logdevice_->log_router_;
  opts.tower.storage = logdevice_->storage_;
  opts.tower.log_router = logdevice_->log_router_;

  EnvOptions env_options;

  if (opts.start_controltower) {
    control_tower_loop_.reset(new MsgLoop(
        env_, env_options, ControlTower::DEFAULT_PORT, 16, info_log_, "tower"));

    // Create ControlTower
    opts.tower.info_log = info_log_;
    opts.tower.number_of_rooms = 16;
    opts.tower.msg_loop = control_tower_loop_.get();
    status_ = ControlTower::CreateNewInstance(opts.tower,
                                              &control_tower_);
    if (!status_.ok()) {
      LOG_ERROR(info_log_, "Failed to create ControlTower.");
      return;
    }

    // Start threads.
    auto entry_point = [] (void* arg) {
      LocalTestCluster* cluster = static_cast<LocalTestCluster*>(arg);
      cluster->control_tower_loop_->Run();
    };
    control_tower_thread_ = env_->StartThread(entry_point, (void *)this,
                                              "tower");

    // Wait for message loop to start.
    status_ = control_tower_loop_->WaitUntilRunning();
    if (!status_.ok()) {
      LOG_ERROR(info_log_, "Failed to start ControlTower (%s)",
        status_.ToString().c_str());
      return;
    }
  }

  if (opts.start_copilot || opts.start_pilot) {
    static_assert(Copilot::DEFAULT_PORT == Pilot::DEFAULT_PORT,
                  "Default port for pilot and copilot differ.");
    cockpit_loop_.reset(new MsgLoop(
        env_, env_options, Copilot::DEFAULT_PORT, 16, info_log_, "cockpit"));

    // If we need to start the copilot, then it is better to start the
    // pilot too. Any subscribe/unsubscribe requests to the copilot needs
    // to write to the rollcall topic (via the pilot).
    opts.start_pilot = true;
    HostId pilot_host("localhost", Pilot::DEFAULT_PORT);
    configuration_.reset(Configuration::Create({pilot_host}, {pilot_host},
                                                SystemTenant));

    if (opts.start_copilot) {
      // Create Copilot
      opts.copilot.control_towers.push_back(control_tower_->GetClientId(0));
      opts.copilot.info_log = info_log_;
      opts.copilot.num_workers = 16;
      opts.copilot.msg_loop = cockpit_loop_.get();
      opts.copilot.control_tower_connections =
          cockpit_loop_->GetNumWorkers();
      opts.copilot.pilots.push_back(pilot_host);
      status_ = Copilot::CreateNewInstance(opts.copilot, &copilot_);
      if (!status_.ok()) {
        LOG_ERROR(info_log_, "Failed to create Copilot.");
        return;
      }
    }

    if (opts.start_pilot) {
      // Create Pilot
      opts.pilot.info_log = info_log_;
      opts.pilot.msg_loop = cockpit_loop_.get();
      status_ = Pilot::CreateNewInstance(opts.pilot, &pilot_);
      if (!status_.ok()) {
        LOG_ERROR(info_log_, "Failed to create Pilot.");
        return;
      }
    }

    auto entry_point = [] (void* arg) {
      LocalTestCluster* cluster = static_cast<LocalTestCluster*>(arg);
      cluster->cockpit_loop_->Run();
    };
    cockpit_thread_ = env_->StartThread(entry_point, (void *)this, "cockpit");

    // Wait for message loop to start.
    status_ = cockpit_loop_->WaitUntilRunning();
    if (!status_.ok()) {
      LOG_ERROR(info_log_, "Failed to start cockpit (%s)",
        status_.ToString().c_str());
      return;
    }
  }
}

Status
LocalTestCluster::CreateClient(const ClientID& id,
                               std::unique_ptr<ClientImpl>* client,
                               bool is_internal) {
  std::unique_ptr<ClientImpl> cl;
  ClientOptions client_options(*configuration_.get(), id);
  Status status = ClientImpl::Create(std::move(client_options),
                                     client, is_internal);
  return status;
}

LocalTestCluster::~LocalTestCluster() {
  // Stop message loops.
  if (cockpit_loop_) {
    cockpit_loop_->Stop();
  }
  if (control_tower_loop_) {
    control_tower_loop_->Stop();
  }

  // Join threads.
  if (cockpit_thread_) {
    env_->WaitForJoin(cockpit_thread_);
  }
  if (control_tower_thread_) {
    env_->WaitForJoin(control_tower_thread_);
  }

  // Delete all components (this stops their worker/room loops).
  delete pilot_;
  delete copilot_;
  delete control_tower_;
}

Statistics LocalTestCluster::GetStatistics() const {
  Statistics aggregated;
  if (pilot_) {
    aggregated.Aggregate(pilot_->GetStatistics());
  }
  if (control_tower_loop_) {
    aggregated.Aggregate(control_tower_loop_->GetStatistics());
  }
  if (cockpit_loop_) {
    aggregated.Aggregate(cockpit_loop_->GetStatistics());
  }
  if (copilot_) {
    aggregated.Aggregate(copilot_->GetStatistics());
  }
  // TODO(pja) 1 : Add control tower once they have stats.
  return std::move(aggregated);
}

std::shared_ptr<LogStorage> LocalTestCluster::GetLogStorage() {
  return logdevice_->storage_;
}

std::shared_ptr<LogRouter> LocalTestCluster::GetLogRouter() {
  return logdevice_->log_router_;
}

}  // namespace rocketspeed
