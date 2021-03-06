// Copyright (c) 2015, Facebook, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
//
#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "include/RocketSpeed.h"

/**
 * This is the Rollcall interface. Applications can use this interface
 * to list all the topics that all clients are subscribed to. It is
 * probable that this interface be a public RocketSpeed interface in
 * the future.
 */
namespace rocketspeed {

/*
 * The callback that is invoked by RocketSpeed for every entry in the
 * rollcall stream.
 */
class RollcallEntry;
typedef std::function<void(RollcallEntry)> RollCallback;

/**
 * Identifies RollCall shard within a namespace. Shards are numbered from 0 to
 * number of shards - 1. All events on a single topic are written to one (and
 * always the same) shard.
 */
typedef uint16_t RollcallShard;

/**
 * The reader that is used to tail the rollcall entries for a namespace.
 */
class RollcallStream {
 public:
  /**
   * Open the rollcall stream for this namespace.
   * @param client_options The options needed to create a client
   * @param tenant_id Tenant ID of rollcall reader.
   * @param reader The reader returned from this call
   * @return the status of whether this call was successful or not
   */
  static Status Open(ClientOptions client_options,
                     TenantID tenant_id,
                     std::unique_ptr<RollcallStream>* reader);

  /** Returns a number of shards for given namespace. */
  virtual RollcallShard GetNumShards(const NamespaceID& namespace_id) = 0;

  /** Subscribes to rollcall updates for given shard in the namespace. */
  virtual Status Subscribe(const NamespaceID& namespace_id,
                           RollcallShard shard_id,
                           RollCallback callback) = 0;

  /**
   * Closes resources associated with this rollcall topic reader
   */
  virtual ~RollcallStream() = default;
};

/*
 * The information returned via every invocation of the RollCallback.
 * Each record is one of SubscriptionRequest/UnsubscriptionRequest/Error.
 * In case of Error, please destroy the RollcallStream to free up
 * all resources asociated with this stream.
 */
class RollcallEntry {
 public:
  static const char ROLLCALL_ENTRY_VERSION_CURRENT = '2';

  // The types of Rollcall Entries.
  enum EntryType : char {
    Error                 = 'E',
    SubscriptionRequest   = 'S',
    UnSubscriptionRequest = 'U',
  };

  RollcallEntry(const Topic& topic, EntryType entry_type) :
    version_(ROLLCALL_ENTRY_VERSION_CURRENT),
    entry_type_(entry_type),
    topic_name_(topic) {
  }
  RollcallEntry() :
    version_(ROLLCALL_ENTRY_VERSION_CURRENT),
    entry_type_(EntryType::Error) {
  }
  explicit RollcallEntry(char version) :
    version_(version),
    entry_type_(EntryType::Error) {
  }
  /*
   * @return the topic name that some client is subscribed to.
   */
  const Topic& GetTopicName() const {
    return topic_name_;
  }

  /*
   * @return EntryType::SubscriptionRequest if this entry
   *                    represents a subscription request,
   *         EntryType::UnSubscriptionRequest if this entry
   *                    represents a unsubscription request,
   *         EntryType::Error if the stream encountered an error
   */
  EntryType GetType() const {
    return entry_type_;
  }

  /*
   * Sets the type of this RollcallEntry
   */
  void SetType(const EntryType& type) {
    entry_type_ = type;
  }

  void Serialize(std::string* buffer);
  Status DeSerialize(Slice* in);
  ~RollcallEntry() = default;

 private:
  char version_;
  EntryType entry_type_;
  Topic topic_name_;
};

inline bool ValidateEnum(RollcallEntry::EntryType e) {
  return e >= RollcallEntry::Error && e <= RollcallEntry::UnSubscriptionRequest;
}

}  // namespace rocketspeed
