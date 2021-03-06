// Copyright (c) 2014, Facebook, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
//
#include "messages.h"

#include <string>
#include <vector>

#include "include/Slice.h"
#include "include/Status.h"
#include "src/util/common/coding.h"

/*
 * This file contains all the messages used by RocketSpeed. These messages are
 * the only means of communication between a client, pilot, copilot and
 * control tower. These are interal to RocketSpeed and can change from one
 * release to another. Applications should not use these messages to communicate
 * with RocketSpeed, instead applications should use the public api specified
 * in include/RocketSpeed.h to interact with RocketSpeed.
 * All messages have to implement the Serializer interface.
 */
namespace rocketspeed {

const char* const kMessageTypeNames[size_t(MessageType::max) + 1] = {
    "invalid",
    "ping",
    "publish",
    "metadata (DEPRECATED)",
    "data_ack",
    "gap",
    "deliver",
    "goodbye",
    "subscribe",
    "unsubscribe",
    "deliver_gap",
    "deliver_data",
    "find_tail_seqno",
    "tail_seqno",
    "deliver_batch",
    "heartbeat",
    "heartbeat_delta",
    "backlog_query",
    "backlog_fill",
    "introduction",
    "deliver_sub_ack",
};

MessageType Message::ReadMessageType(Slice slice) {
  MessageType mtype;
  if (slice.size() < sizeof(mtype)) {
    return MessageType::NotInitialized;
  }
  memcpy(&mtype, slice.data(), sizeof(mtype));
  return mtype;
}

 /**
  * Creates a Message of the appropriate subtype by looking at the
  * MessageType. Returns nullptr on error. It is the responsibility
  * of the caller to own this memory object.
  **/
std::unique_ptr<Message>
Message::CreateNewInstance(Slice* in) {
  MessageType mtype = ReadMessageType(*in);
  if (mtype == MessageType::NotInitialized) {
    return nullptr;
  }

  Status st;
  switch (mtype) {
    case MessageType::mPing: {
      std::unique_ptr<MessagePing> msg(new MessagePing());
      st = msg->DeSerialize(in);
      if (st.ok()) {
        return std::unique_ptr<Message>(msg.release());
      }
      break;
    }

    case MessageType::mPublish:
    case MessageType::mDeliver: {
      std::unique_ptr<MessageData> msg(new MessageData());
      st = msg->DeSerialize(in);
      if (st.ok()) {
        return std::unique_ptr<Message>(msg.release());
      }
      break;
    }

    case MessageType::mDataAck: {
      std::unique_ptr<MessageDataAck> msg(new MessageDataAck());
      st = msg->DeSerialize(in);
      if (st.ok()) {
        return std::unique_ptr<Message>(msg.release());
      }
      break;
    }

    case MessageType::mGap: {
      std::unique_ptr<MessageGap> msg(new MessageGap());
      st = msg->DeSerialize(in);
      if (st.ok()) {
        return std::unique_ptr<Message>(msg.release());
      }
      break;
    }

    case MessageType::mGoodbye: {
      std::unique_ptr<MessageGoodbye> msg(new MessageGoodbye());
      st = msg->DeSerialize(in);
      if (st.ok()) {
        return std::unique_ptr<Message>(msg.release());
      }
      break;
    }

    case MessageType::mSubscribe: {
      std::unique_ptr<MessageSubscribe> msg(new MessageSubscribe());
      st = msg->DeSerialize(in);
      if (st.ok()) {
        return std::unique_ptr<Message>(msg.release());
      }
      break;
    }

    case MessageType::mUnsubscribe: {
      std::unique_ptr<MessageUnsubscribe> msg(new MessageUnsubscribe());
      st = msg->DeSerialize(in);
      if (st.ok()) {
        return std::unique_ptr<Message>(msg.release());
      }
      break;
    }

    case MessageType::mDeliverGap: {
      std::unique_ptr<MessageDeliverGap> msg(new MessageDeliverGap());
      st = msg->DeSerialize(in);
      if (st.ok()) {
        return std::unique_ptr<Message>(msg.release());
      }
      break;
    }

    case MessageType::mDeliverData: {
      std::unique_ptr<MessageDeliverData> msg(new MessageDeliverData());
      st = msg->DeSerialize(in);
      if (st.ok()) {
        return std::unique_ptr<Message>(msg.release());
      }
      break;
    }

    case MessageType::mFindTailSeqno: {
      std::unique_ptr<MessageFindTailSeqno> msg(new MessageFindTailSeqno());
      st = msg->DeSerialize(in);
      if (st.ok()) {
        return std::unique_ptr<Message>(msg.release());
      }
      break;
    }

    case MessageType::mTailSeqno: {
      std::unique_ptr<MessageTailSeqno> msg(new MessageTailSeqno());
      st = msg->DeSerialize(in);
      if (st.ok()) {
        return std::unique_ptr<Message>(msg.release());
      }
      break;
    }

    case MessageType::mDeliverBatch: {
      std::unique_ptr<MessageDeliverBatch> msg(new MessageDeliverBatch());
      st = msg->DeSerialize(in);
      if (st.ok()) {
        return std::unique_ptr<Message>(msg.release());
      }
      break;
    }

    case MessageType::mHeartbeat: {
      std::unique_ptr<MessageHeartbeat> msg(new MessageHeartbeat());
      st = msg->DeSerialize(in);
      if (st.ok()) {
        return std::unique_ptr<Message>(msg.release());
      }
      break;
    }

    case MessageType::mHeartbeatDelta: {
      std::unique_ptr<MessageHeartbeatDelta> msg(new MessageHeartbeatDelta());
      st = msg->DeSerialize(in);
      if (st.ok()) {
        return std::unique_ptr<Message>(msg.release());
      }
      break;
    }

    case MessageType::mBacklogQuery: {
      std::unique_ptr<MessageBacklogQuery> msg(new MessageBacklogQuery());
      st = msg->DeSerialize(in);
      if (st.ok()) {
        return std::unique_ptr<Message>(msg.release());
      }
      break;
    }

    case MessageType::mBacklogFill: {
      std::unique_ptr<MessageBacklogFill> msg(new MessageBacklogFill());
      st = msg->DeSerialize(in);
      if (st.ok()) {
        return std::unique_ptr<Message>(msg.release());
      }
      break;
    }

    case MessageType::mIntroduction: {
      std::unique_ptr<MessageIntroduction> msg(new MessageIntroduction());
      st = msg->DeSerialize(in);
      if (st.ok()) {
        return std::unique_ptr<Message>(msg.release());
      }
      break;
    }

    case MessageType::mSubAck: {
      std::unique_ptr<MessageSubAck> msg(new MessageSubAck());
      st = msg->DeSerialize(in);
      if (st.ok()) {
        return std::unique_ptr<Message>(msg.release());
      }
      break;
    }

    default:
      break;
  }
  return nullptr;
}

std::unique_ptr<Message> Message::CreateNewInstance(Slice in) {
  return CreateNewInstance(&in);
}

std::unique_ptr<Message> Message::Copy(const Message& msg) {
  // Not efficient, but not used often, and should be efficient once we
  // have shared serialized buffers.
  std::string serial;
  msg.SerializeToString(&serial);
  Slice slice(serial);
  return CreateNewInstance(&slice);
}

Status Message::Serialize(std::string* out) const {
  PutFixedEnum8(out, type_);
  PutFixed16(out, tenantid_);
  return Status::OK();
}

Status Message::DeSerialize(Slice* in) {
  if (!GetFixedEnum8(in, &type_)) {
    return Status::InvalidArgument("Bad MessageType");
  }
  if (!GetFixed16(in, &tenantid_)) {
    return Status::InvalidArgument("Bad TenantID");
  }
  return Status::OK();
}

void Message::SerializeToString(std::string* out) const {
  Serialize(out);
}

Status MessagePing::Serialize(std::string* out) const {
  PutFixedEnum8(out, type_);
  PutFixed16(out, tenantid_);

  // serialize message specific contents
  // pingtype
  PutFixedEnum8(out, pingtype_);
  // cookie
  PutLengthPrefixedSlice(out, Slice(cookie_));
  return Status::OK();
}

Status MessagePing::DeSerialize(Slice* in) {
  // extract type
  if (!GetFixedEnum8(in, &type_)) {
    return Status::InvalidArgument("Bad type");
  }

  if (!GetFixed16(in, &tenantid_)) {
    return Status::InvalidArgument("Bad tenant ID");
  }

  // extract ping type
  if (!GetFixedEnum8(in, &pingtype_)) {
    return Status::InvalidArgument("Bad ping type");
  }

  // extract cookie
  if (!GetLengthPrefixedSlice(in, &cookie_)) {
    return Status::InvalidArgument("Bad cookie");
  }
  return Status::OK();
}

MessageData::MessageData(MessageType type,
                         TenantID tenantID,
                         Topic topic_name,
                         NamespaceID namespace_id,
                         std::string payload) :
  Message(type, tenantID),
  topic_name_(std::move(topic_name)),
  payload_(std::move(payload)),
  namespaceid_(std::move(namespace_id)) {
  RS_ASSERT(type == MessageType::mPublish || type == MessageType::mDeliver);
  seqno_ = 0;
  seqno_prev_ = 0;
}

MessageData::MessageData(MessageType type):
  MessageData(type, Tenant::InvalidTenant, "",
              InvalidNamespace, "") {
}

MessageData::MessageData():
  MessageData(MessageType::mPublish) {
}

MessageData::~MessageData() {
}

Status MessageData::Serialize(std::string* out) const {
  PutFixedEnum8(out, type_);

  // seqno
  PutVarint64(out, seqno_prev_);
  PutVarint64(out, seqno_);

  // The rest of the message is what goes into log storage.
  SerializeInternal(out);
  return Status::OK();
}

Status MessageData::DeSerialize(Slice* in) {
  // extract type
  if (!GetFixedEnum8(in, &type_)) {
    return Status::InvalidArgument("Bad type");
  }

  // extract sequence numbers of message
  if (!GetVarint64(in, &seqno_prev_)) {
    return Status::InvalidArgument("Bad Previous Sequence Number");
  }

  if (!GetVarint64(in, &seqno_)) {
    return Status::InvalidArgument("Bad Sequence Number");
  }

  // The rest of the message is what goes into log storage.
  return DeSerializeStorage(in);
}

std::string MessageData::GetStorage() const {
  std::string storage;
  SerializeInternal(&storage);
  return storage;
}

size_t MessageData::GetTotalSize() const {
  return sizeof(MessageData) + topic_name_.size() +
         payload_.size() + namespaceid_.size() + payload_.size();
}

void MessageData::SerializeInternal(std::string* out) const {
  PutFixed16(out, tenantid_);
  PutTopicID(out, namespaceid_, topic_name_);
  PutLengthPrefixedSlice(out,
                         Slice((const char*)&msgid_, sizeof(msgid_)));

  PutLengthPrefixedSlice(out, payload_);
}

Status MessageData::DeSerializeStorage(Slice* in) {
  // extract tenant ID
  if (!GetFixed16(in, &tenantid_)) {
    return Status::InvalidArgument("Bad tenant ID");
  }

  // extract message topic
  if (!GetTopicID(in, &namespaceid_, &topic_name_)) {
    return Status::InvalidArgument("Bad Message Topic ID");
  }

  // extract message id
  Slice idSlice;
  if (!GetLengthPrefixedSlice(in, &idSlice) ||
      idSlice.size() < sizeof(msgid_)) {
    return Status::InvalidArgument("Bad Message Id");
  }
  memcpy(&msgid_, idSlice.data(), sizeof(msgid_));

  // extract payload (the rest of the message)
  if (!GetLengthPrefixedSlice(in, &payload_)) {
    return Status::InvalidArgument("Bad payload");
  }
  return Status::OK();
}

MessageDataAck::MessageDataAck(TenantID tenantID,
                               AckVector acks)
: acks_(std::move(acks)) {
  type_ = MessageType::mDataAck;
  tenantid_ = tenantID;
}

MessageDataAck::~MessageDataAck() {
}

const MessageDataAck::AckVector& MessageDataAck::GetAcks() const {
  return acks_;
}

Status MessageDataAck::Serialize(std::string* out) const {
  // Type, tenantId and origin
  PutFixedEnum8(out, type_);
  PutFixed16(out, tenantid_);

  // serialize message specific contents
  PutVarint32(out, static_cast<uint32_t>(acks_.size()));
  for (const Ack& ack : acks_) {
    PutFixedEnum8(out, ack.status);
    PutBytes(out, ack.msgid.id, sizeof(ack.msgid.id));
    PutVarint64(out, ack.seqno);
  }
  return Status::OK();
}

Status MessageDataAck::DeSerialize(Slice* in) {
  // extract type
  if (!GetFixedEnum8(in, &type_)) {
    return Status::InvalidArgument("Bad type");
  }

  // extrant tenant ID
  if (!GetFixed16(in, &tenantid_)) {
    return Status::InvalidArgument("Bad tenant ID");
  }

  // extract number of acks
  uint32_t num_acks;
  if (!GetVarint32(in, &num_acks)) {
    return Status::InvalidArgument("Bad Number Of Acks");
  }

  // extract each ack
  for (unsigned i = 0; i < num_acks; i++) {
    Ack ack;

    // extract status
    if (!GetFixedEnum8(in, &ack.status)) {
      return Status::InvalidArgument("Bad Ack Status");
    }

    // extract msgid
    if (!GetBytes(in, ack.msgid.id, sizeof(ack.msgid.id))) {
      return Status::InvalidArgument("Bad Ack MsgId");
    }

    if (!GetVarint64(in, &ack.seqno)) {
      return Status::InvalidArgument("Bad Ack Sequence number");
    }

    acks_.push_back(ack);
  }

  return Status::OK();
}

MessageGap::MessageGap(TenantID tenantID,
                       NamespaceID namespace_id,
                       Topic topic_name,
                       GapType gap_type,
                       SequenceNumber gap_from,
                       SequenceNumber gap_to)
: namespace_id_(std::move(namespace_id))
, topic_name_(std::move(topic_name))
, gap_type_(gap_type)
, gap_from_(gap_from)
, gap_to_(gap_to) {
  type_ = MessageType::mGap;
  tenantid_ = tenantID;
}

MessageGap::~MessageGap() {
}

Status MessageGap::Serialize(std::string* out) const {
  // Type, tenantId and origin
  PutFixedEnum8(out, type_);
  PutFixed16(out, tenantid_);

  // Write the gap information.
  PutTopicID(out, Slice(namespace_id_), Slice(topic_name_));
  PutFixedEnum8(out, gap_type_);
  PutVarint64(out, gap_from_);
  PutVarint64(out, gap_to_);
  return Status::OK();
}

Status MessageGap::DeSerialize(Slice* in) {
  // extract type
  if (!GetFixedEnum8(in, &type_)) {
    return Status::InvalidArgument("Bad type");
  }

  // extrant tenant ID
  if (!GetFixed16(in, &tenantid_)) {
    return Status::InvalidArgument("Bad tenant ID");
  }

  // Read topic ID.
  if (!GetTopicID(in, &namespace_id_, &topic_name_)) {
    return Status::InvalidArgument("Invalid topic ID");
  }

  // Read gap type
  if (!GetFixedEnum8(in, &gap_type_)) {
    return Status::InvalidArgument("Missing gap type");
  }

  // Read gap start seqno
  if (!GetVarint64(in, &gap_from_)) {
    return Status::InvalidArgument("Bad gap log ID");
  }

  // Read gap end seqno
  if (!GetVarint64(in, &gap_to_)) {
    return Status::InvalidArgument("Bad gap log ID");
  }
  return Status::OK();
}

MessageGoodbye::MessageGoodbye(TenantID tenant_id,
                               Code code,
                               OriginType origin_type)
: code_(code)
, origin_type_(origin_type) {
  type_ = MessageType::mGoodbye;
  tenantid_ = tenant_id;
}

Status MessageGoodbye::Serialize(std::string* out) const {
  // Type, tenantId and origin
  PutFixedEnum8(out, type_);
  PutFixed16(out, tenantid_);

  // MessageGoodbye specifics
  PutFixedEnum8(out, code_);
  PutFixedEnum8(out, origin_type_);
  return Status::OK();
}

Status MessageGoodbye::DeSerialize(Slice* in) {
  // extract type
  if (!GetFixedEnum8(in, &type_)) {
    return Status::InvalidArgument("Bad type");
  }

  // extrant tenant ID
  if (!GetFixed16(in, &tenantid_)) {
    return Status::InvalidArgument("Bad tenant ID");
  }

  // extract code
  if (!GetFixedEnum8(in, &code_)) {
    return Status::InvalidArgument("Bad code");
  }

  // extract origin type
  if (!GetFixedEnum8(in, &origin_type_)) {
    return Status::InvalidArgument("Bad origin type");
  }

  return Status::OK();
}

Status MessageFindTailSeqno::Serialize(std::string* out) const {
  Message::Serialize(out);
  PutTopicID(out, namespace_id_, topic_name_);
  return Status::OK();
}

Status MessageFindTailSeqno::DeSerialize(Slice* in) {
  Status st = Message::DeSerialize(in);
  if (!st.ok()) {
    return st;
  }
  if (!GetTopicID(in, &namespace_id_, &topic_name_)) {
    return Status::InvalidArgument("Bad NamespaceID and/or TopicName");
  }
  return Status::OK();
}

Status MessageTailSeqno::Serialize(std::string* out) const {
  Message::Serialize(out);
  PutTopicID(out, namespace_id_, topic_name_);
  PutVarint64(out, seqno_);
  return Status::OK();
}

Status MessageTailSeqno::DeSerialize(Slice* in) {
  Status st = Message::DeSerialize(in);
  if (!st.ok()) {
    return st;
  }
  if (!GetTopicID(in, &namespace_id_, &topic_name_)) {
    return Status::InvalidArgument("Bad NamespaceID and/or TopicName");
  }
  if (!GetVarint64(in, &seqno_)) {
    return Status::InvalidArgument("Bad sequence number");
  }
  return Status::OK();
}

////////////////////////////////////////////////////////////////////////////////
Status MessageSubscribe::Serialize(std::string* out) const {
  Message::Serialize(out);
  PutTopicID(out, namespace_id_, topic_name_);
  if (start_.size() >= 1) {
    PutVarint64(out, start_[0].seqno);
  } else {
    PutVarint64(out, 0);  // backwards compat, used to be seqno
  }
  EncodeSubscriptionID(out, sub_id_);
  size_t num_cursors = start_.size();
  PutVarint64(out, num_cursors);
  for (const auto& cursor : start_) {
    PutLengthPrefixedSlice(out, cursor.source);
  }
  for (const auto& cursor : start_) {
    PutVarint64(out, cursor.seqno);
  }
  return Status::OK();
}

Status MessageSubscribe::DeSerialize(Slice* in) {
  Status st = Message::DeSerialize(in);
  if (!st.ok()) {
    return st;
  }
  if (!GetTopicID(in, &namespace_id_, &topic_name_)) {
    return Status::InvalidArgument("Bad NamespaceID and/or TopicName");
  }
  SequenceNumber start_seqno;
  if (!GetVarint64(in, &start_seqno)) {
    return Status::InvalidArgument("Bad SequenceNumber");
  }
  if (!DecodeSubscriptionID(in, &sub_id_)) {
    return Status::InvalidArgument("Bad SubscriptionID");
  }
  uint64_t num_cursors;
  if (GetVarint64(in, &num_cursors)) {
    start_.resize(num_cursors);
    for (uint64_t i = 0; i < num_cursors; ++i) {
      if (!GetLengthPrefixedSlice(in, &start_[i].source)) {
        return Status::InvalidArgument("Bad cursor source");
      }
    }
    for (uint64_t i = 0; i < num_cursors; ++i) {
      if (!GetVarint64(in, &start_[i].seqno)) {
        return Status::InvalidArgument("Bad cursor seqno");
      }
    }
  } else {
    // OK, for backwards compat.
    // Old message format, so use start_seqno on empty topic.
    // TODO(pja): Make this an error once required.
    start_.emplace_back("", start_seqno);
  }
  return Status::OK();
}

Status MessageUnsubscribe::Serialize(std::string* out) const {
  Message::Serialize(out);
  EncodeSubscriptionID(out, sub_id_);
  PutFixedEnum8(out, reason_);
  PutTopicID(out, namespace_id_, topic_name_);
  return Status::OK();
}

Status MessageUnsubscribe::DeSerialize(Slice* in) {
  Status st = Message::DeSerialize(in);
  if (!st.ok()) {
    return st;
  }
  if (!DecodeSubscriptionID(in, &sub_id_)) {
    return Status::InvalidArgument("Bad SubscriptionID");
  }
  if (!GetFixedEnum8(in, &reason_)) {
    return Status::InvalidArgument("Bad Reason");
  }
  if (!GetTopicID(in, &namespace_id_, &topic_name_)) {
    // We allow this for backwards compatibility.
    // TODO(pja): Make this an error once required.
    namespace_id_.clear();
    topic_name_.clear();
  }
  return Status::OK();
}

Status MessageDeliver::Serialize(std::string* out) const {
  Message::Serialize(out);
  EncodeSubscriptionID(out, sub_id_);
  PutVarint64(out, seqno_prev_);
  RS_ASSERT(seqno_ >= seqno_prev_);
  uint64_t seqno_diff = seqno_ - seqno_prev_;
  PutVarint64(out, seqno_diff);
  return Status::OK();
}

Status MessageDeliver::DeSerialize(Slice* in) {
  Status st = Message::DeSerialize(in);
  if (!st.ok()) {
    return st;
  }
  if (!DecodeSubscriptionID(in, &sub_id_)) {
    return Status::InvalidArgument("Bad SubscriptionID");
  }
  if (!GetVarint64(in, &seqno_prev_)) {
    return Status::InvalidArgument("Bad previous SequenceNumber");
  }
  uint64_t seqno_diff;
  if (!GetVarint64(in, &seqno_diff)) {
    return Status::InvalidArgument("Bad difference between SequenceNumbers");
  }
  seqno_ = seqno_prev_ + seqno_diff;
  RS_ASSERT(seqno_ >= seqno_prev_);
  return Status::OK();
}

Status MessageDeliverGap::Serialize(std::string* out) const {
  MessageDeliver::Serialize(out);
  PutFixedEnum8(out, gap_type_);
  PutTopicID(out, namespace_id_, topic_);
  PutLengthPrefixedSlice(out, source_);
  return Status::OK();
}

Status MessageDeliverGap::DeSerialize(Slice* in) {
  Status st = MessageDeliver::DeSerialize(in);
  if (!st.ok()) {
    return st;
  }
  if (!GetFixedEnum8(in, &gap_type_)) {
    return Status::InvalidArgument("Bad GapType");
  }
  if (!GetTopicID(in, &namespace_id_, &topic_)) {
    // OK, for backwards compat.
    // TODO(pja): Make this an error once required.
    namespace_id_.clear();
    topic_.clear();
  }
  if (!GetLengthPrefixedSlice(in, &source_)) {
    // OK, for backwards compat.
    // TODO(pja): Make this an error once required.
  }
  return Status::OK();
}

Status MessageSubAck::Serialize(std::string* out) const {
  Message::Serialize(out);
  PutTopicID(out, namespace_id_, topic_);
  EncodeSubscriptionID(out, sub_id_);
  size_t num_cursors = cursors_.size();
  PutVarint64(out, num_cursors);
  for (const auto& cursor : cursors_) {
    PutLengthPrefixedSlice(out, cursor.source);
  }
  for (const auto& cursor : cursors_) {
    PutVarint64(out, cursor.seqno);
  }
  return Status::OK();
}

Status MessageSubAck::DeSerialize(Slice* in) {
  Status st = Message::DeSerialize(in);
  if (!st.ok()) {
    return st;
  }
  if (!GetTopicID(in, &namespace_id_, &topic_)) {
    return Status::InvalidArgument("Bad NamespaceID and/or TopicName");
  }
  if (!DecodeSubscriptionID(in, &sub_id_)) {
    return Status::InvalidArgument("Bad SubscriptionID");
  }
  uint64_t num_cursors;
  if (GetVarint64(in, &num_cursors)) {
    cursors_.resize(num_cursors);
    for (uint64_t i = 0; i < num_cursors; ++i) {
      if (!GetLengthPrefixedSlice(in, &cursors_[i].source)) {
        return Status::InvalidArgument("Bad cursor source");
      }
    }
    for (uint64_t i = 0; i < num_cursors; ++i) {
      if (!GetVarint64(in, &cursors_[i].seqno)) {
        return Status::InvalidArgument("Bad cursor seqno");
      }
    }
  }
  return Status::OK();
}

Status MessageDeliverData::Serialize(std::string* out) const {
  MessageDeliver::Serialize(out);
  PutLengthPrefixedSlice(out,
                         Slice((const char*)&message_id_, sizeof(message_id_)));
  PutLengthPrefixedSlice(out, payload_);
  PutTopicID(out, namespace_id_, topic_);
  PutLengthPrefixedSlice(out, source_);
  return Status::OK();
}

Status MessageDeliverData::DeSerialize(Slice* in) {
  Status st = MessageDeliver::DeSerialize(in);
  if (!st.ok()) {
    return st;
  }
  Slice id_slice;
  if (!GetLengthPrefixedSlice(in, &id_slice) ||
      id_slice.size() < sizeof(message_id_)) {
    return Status::InvalidArgument("Bad Message ID");
  }
  memcpy(&message_id_, id_slice.data(), sizeof(message_id_));
  if (!GetLengthPrefixedSlice(in, &payload_)) {
    return Status::InvalidArgument("Bad payload");
  }
  if (!GetTopicID(in, &namespace_id_, &topic_)) {
    // OK, for backwards compat.
    // TODO(pja): Make this an error once required.
    namespace_id_.clear();
    topic_.clear();
  }
  if (!GetLengthPrefixedSlice(in, &source_)) {
    // OK, for backwards compat.
    // TODO(pja): Make this an error once required.
  }
  return Status::OK();
}

Status MessageDeliverBatch::Serialize(std::string* out) const {
  Message::Serialize(out);
  PutVarint64(out, messages_.size());
  for (const auto& msg : messages_) {
    std::string one;
    Status st = msg->Serialize(&one);
    if (!st.ok()) {
      return Status::InvalidArgument("Bad MessageDeliverData");
    }
    PutLengthPrefixedSlice(out, one);
  }
  return Status::OK();
}

Status MessageDeliverBatch::DeSerialize(Slice* in) {
  Status st = Message::DeSerialize(in);
  if (!st.ok()) {
    return st;
  }
  uint64_t len;
  if (!GetVarint64(in, &len)) {
    return Status::InvalidArgument("Bad Messages count");
  }
  messages_.clear();
  messages_.reserve(len);
  for (size_t i = 0; i < len; ++i) {
    messages_.emplace_back(new MessageDeliverData());
    Slice one;
    if (!GetLengthPrefixedSlice(in, &one)) {
      return Status::InvalidArgument("Bad sub-message");
    }
    st = messages_.back()->DeSerialize(&one);
    if (!st.ok()) {
      return st;
    }
  }
  return Status::OK();
}

Status MessageHeartbeat::Serialize(std::string* out) const {
  using namespace std::chrono;
  PutFixedEnum8(out, type_);
  PutFixed16(out, tenantid_);

  uint64_t source_ms = duration_cast<milliseconds>(
    timestamp_.time_since_epoch())
    .count();
  PutFixed64(out, source_ms);

  // Check that heartbeats are strictly sorted.
  RS_ASSERT_DBG(std::is_sorted(
      healthy_streams_.begin(),
      healthy_streams_.end(),
      std::less_equal<uint32_t>()));
  for (uint32_t shard : healthy_streams_) {
    PutVarint32(out, shard);
  }

  return Status::OK();
}

Status MessageHeartbeat::DeSerialize(Slice* in) {
  // extract type
  if (!GetFixedEnum8(in, &type_)) {
    return Status::InvalidArgument("Bad type");
  }

  if (!GetFixed16(in, &tenantid_)) {
    return Status::InvalidArgument("Bad tenant ID");
  }

  if (in->size() == 0) {
    return Status::OK();        // for backwards compatibility
  }

  uint64_t source_ms;
  if (!GetFixed64(in, &source_ms)) {
    return Status::InvalidArgument("Bad timestamp");
  }
  Clock::duration since_source = std::chrono::milliseconds(source_ms);
  timestamp_ = Clock::time_point(since_source);

  uint32_t shard;
  while (GetVarint32(in, &shard)) {
    healthy_streams_.push_back(shard);
  }

  return Status::OK();
}

Status MessageHeartbeatDelta::Serialize(std::string* out) const {
  using namespace std::chrono;
  PutFixedEnum8(out, type_);
  PutFixed16(out, tenantid_);

  uint64_t source_ms = duration_cast<milliseconds>(
    timestamp_.time_since_epoch())
    .count();
  PutFixed64(out, source_ms);

  // Check that heartbeats are strictly sorted.
  RS_ASSERT_DBG(std::is_sorted(
      added_healthy_.begin(),
      added_healthy_.end(),
      std::less_equal<uint32_t>()));
  RS_ASSERT_DBG(std::is_sorted(
      removed_healthy_.begin(),
      removed_healthy_.end(),
      std::less_equal<uint32_t>()));

  PutVarint64(out, added_healthy_.size());
  for (uint32_t shard : added_healthy_) {
    PutVarint32(out, shard);
  }

  PutVarint64(out, removed_healthy_.size());
  for (uint32_t shard : removed_healthy_) {
    PutVarint32(out, shard);
  }

  return Status::OK();
}

Status MessageHeartbeatDelta::DeSerialize(Slice* in) {
  // extract type
  if (!GetFixedEnum8(in, &type_)) {
    return Status::InvalidArgument("Bad type");
  }

  if (!GetFixed16(in, &tenantid_)) {
    return Status::InvalidArgument("Bad tenant ID");
  }

  uint64_t source_ms;
  if (!GetFixed64(in, &source_ms)) {
    return Status::InvalidArgument("Bad timestamp");
  }
  Clock::duration since_source = std::chrono::milliseconds(source_ms);
  timestamp_ = Clock::time_point(since_source);

  uint64_t num_added;
  if (!GetVarint64(in, &num_added)) {
    return Status::InvalidArgument("Bad num_added");
  }

  added_healthy_.reserve(num_added);
  while (num_added--) {
    uint32_t shard;
    if (!GetVarint32(in, &shard)) {
      return Status::InvalidArgument("Bad added shard");
    }
    added_healthy_.push_back(shard);
  }

  uint64_t num_removed;
  if (!GetVarint64(in, &num_removed)) {
    return Status::InvalidArgument("Bad num_removed");
  }

  removed_healthy_.reserve(num_removed);
  while (num_removed--) {
    uint32_t shard;
    if (!GetVarint32(in, &shard)) {
      return Status::InvalidArgument("Bad removed shard");
    }
    removed_healthy_.push_back(shard);
  }

  return Status::OK();
}

Status MessageBacklogQuery::Serialize(std::string* out) const {
  PutFixedEnum8(out, type_);
  PutFixed16(out, tenantid_);
  EncodeSubscriptionID(out, sub_id_);
  PutTopicID(out, namespace_id_, topic_);
  PutLengthPrefixedSlice(out, source_);
  PutVarint64(out, seqno_);
  return Status::OK();
}

Status MessageBacklogQuery::DeSerialize(Slice* in) {
  if (!GetFixedEnum8(in, &type_)) {
    return Status::InvalidArgument("Bad type");
  }
  if (!GetFixed16(in, &tenantid_)) {
    return Status::InvalidArgument("Bad tenant ID");
  }
  if (!DecodeSubscriptionID(in, &sub_id_)) {
    return Status::InvalidArgument("Bad sub ID");
  }
  if (!GetTopicID(in, &namespace_id_, &topic_)) {
    return Status::InvalidArgument("Bad namespace and/or topic");
  }
  if (!GetLengthPrefixedSlice(in, &source_)) {
    return Status::InvalidArgument("Bad source");
  }
  if (!GetVarint64(in, &seqno_)) {
    return Status::InvalidArgument("Bad seqno");
  }
  return Status::OK();
}

Status MessageBacklogFill::Serialize(std::string* out) const {
  PutFixedEnum8(out, type_);
  PutFixed16(out, tenantid_);
  PutTopicID(out, namespace_id_, topic_);
  PutLengthPrefixedSlice(out, source_);
  PutVarint64(out, prev_seqno_);
  PutVarint64(out, next_seqno_);
  PutFixedEnum8(out, result_);
  PutLengthPrefixedSlice(out, info_);
  return Status::OK();
}

Status MessageBacklogFill::DeSerialize(Slice* in) {
  if (!GetFixedEnum8(in, &type_)) {
    return Status::InvalidArgument("Bad type");
  }
  if (!GetFixed16(in, &tenantid_)) {
    return Status::InvalidArgument("Bad tenant ID");
  }
  if (!GetTopicID(in, &namespace_id_, &topic_)) {
    return Status::InvalidArgument("Bad namespace and/or topic");
  }
  if (!GetLengthPrefixedSlice(in, &source_)) {
    return Status::InvalidArgument("Bad source");
  }
  if (!GetVarint64(in, &prev_seqno_)) {
    return Status::InvalidArgument("Bad prev seqno");
  }
  if (!GetVarint64(in, &next_seqno_)) {
    return Status::InvalidArgument("Bad next seqno");
  }
  if (!GetFixedEnum8(in, &result_)) {
    return Status::InvalidArgument("Bad result");
  }
  if (!GetLengthPrefixedSlice(in, &info_)) {
    // Info may not be there for backwards compatibility.
    info_.clear();
  }
  return Status::OK();
}

Status MessageIntroduction::Serialize(std::string* out) const {
  Message::Serialize(out);

  // Stream Properties
  PutVarint64(out, stream_properties_.size());
  for (const auto& info : stream_properties_) {
    PutLengthPrefixedSlice(out, info.first);
    PutLengthPrefixedSlice(out, info.second);
  }

  // Client Properties
  PutVarint64(out, client_properties_.size());
  for (const auto& info : client_properties_) {
    PutLengthPrefixedSlice(out, info.first);
    PutLengthPrefixedSlice(out, info.second);
  }
  return Status::OK();
}

Status MessageIntroduction::DeSerialize(Slice* in) {
  Status st = Message::DeSerialize(in);
  if (!st.ok()) {
    return st;
  }

  // Stream Properties
  uint64_t len_stream;
  if (!GetVarint64(in, &len_stream)) {
    return Status::InvalidArgument("Bad Properties count");
  }
  stream_properties_.clear();
  stream_properties_.reserve(len_stream);
  for (size_t i = 0; i < len_stream; i++) {
    Slice key;
    Slice value;
    if (!GetLengthPrefixedSlice(in, &key)) {
      return Status::InvalidArgument("Bad property key");
    }
    if (!GetLengthPrefixedSlice(in, &value)) {
      return Status::InvalidArgument("Bad property value");
    }
    stream_properties_.emplace(key.ToString(), value.ToString());
  }

  // Client Properties
  uint64_t len_client;
  if (!GetVarint64(in, &len_client)) {
    return Status::InvalidArgument("Bad Properties count");
  }
  client_properties_.clear();
  client_properties_.reserve(len_client);
  for (size_t i = 0; i < len_client; i++) {
    Slice key;
    Slice value;
    if (!GetLengthPrefixedSlice(in, &key)) {
      return Status::InvalidArgument("Bad property key");
    }
    if (!GetLengthPrefixedSlice(in, &value)) {
      return Status::InvalidArgument("Bad property value");
    }
    client_properties_.emplace(key.ToString(), value.ToString());
  }

  return Status::OK();
}

}  // namespace rocketspeed
