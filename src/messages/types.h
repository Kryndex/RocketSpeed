//  Copyright (c) 2015, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
//
#pragma once

#include <memory>
#include <string>

namespace rocketspeed {

class Message;
class Slice;

/**
 * Identifies a stream, which is a pair of unidirectional channels, one in each
 * direction. Messages flowing in one direction within given stream are linearly
 * ordered. Two messages flowing in opposite directions have no ordering
 * guarantees.
 * The ID uniquely identifies a stream within a single physical connection only,
 * that means if streams are multiplexed on the same connection and have the
 * same IDs, the IDs need to be remapped. The IDs do not need to be unique
 * system-wide.
 */
typedef unsigned long long int StreamID;
static_assert(sizeof(StreamID) == 8, "Invalid StreamID size.");

/**
 * Encodes stream ID onto wire.
 *
 * @param out Output string to append encoded origin to.
 * @param origin Origin stream ID.
 */
void EncodeOrigin(std::string* out, StreamID origin);

/**
 * Decodes wire format of stream origin.
 *
 * @param in Input slice of encoded stream spec. Will be advanced beyond spec.
 * @param origin Output parameter for decoded stream.
 * @return true iff successfully decoded.
 */
bool DecodeOrigin(Slice* in, StreamID* origin);

struct TimestampedString {
  std::string string;
  uint64_t issued_time;
};

typedef std::shared_ptr<TimestampedString> SharedTimestampedString;

struct MessageOnStream {
  StreamID stream_id;
  std::unique_ptr<Message> message;
};

struct SerialisedOnStream {
  StreamID stream_id;
  SharedTimestampedString serialised;
};

}  // namespace rocketspeed