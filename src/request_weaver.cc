// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////////
//
#include "grpc_transcoding/request_weaver.h"

#include <string>
#include <vector>

#include "absl/strings/str_format.h"

#include "google/protobuf/stubs/mathutil.h"
#include "google/protobuf/stubs/strutil.h"
#include "google/protobuf/type.pb.h"
#include "google/protobuf/util/internal/datapiece.h"
#include "google/protobuf/util/internal/object_writer.h"

namespace google {
namespace grpc {

namespace transcoding {

namespace pb = google::protobuf;
namespace pbconv = google::protobuf::util::converter;

namespace {

pb::util::Status bindingFailureStatus(internal::string_view field_name,
                                      internal::string_view type,
                                      const pbconv::DataPiece& value) {
  return pb::util::Status(
      pb::util::StatusCode::kInvalidArgument,
      pb::StrCat("Failed to convert binding value ", field_name, ":",
                 value.ValueAsStringOrDefault(""), " to ", type));
}

pb::util::Status isEqual(internal::string_view field_name,
                         const pbconv::DataPiece& value_in_body,
                         const pbconv::DataPiece& value_in_binding) {
  bool value_is_same = true;
  switch (value_in_body.type()) {
    case pbconv::DataPiece::TYPE_INT32: {
      pb::util::StatusOr<int32_t> status = value_in_binding.ToInt32();
      if (!status.ok()) {
        return bindingFailureStatus(field_name, "int32", value_in_binding);
      }
      if (status.value() != value_in_body.ToInt32().value()) {
        value_is_same = false;
      }
      break;
    }
    case pbconv::DataPiece::TYPE_INT64: {
      pb::util::StatusOr<uint32_t> status = value_in_binding.ToInt64();
      if (!status.ok()) {
        return bindingFailureStatus(field_name, "int64", value_in_binding);
      }
      if (status.value() != value_in_body.ToInt64().value()) {
        value_is_same = false;
      }
      break;
    }
    case pbconv::DataPiece::TYPE_UINT32: {
      pb::util::StatusOr<uint32_t> status = value_in_binding.ToUint32();
      if (!status.ok()) {
        return bindingFailureStatus(field_name, "uint32", value_in_binding);
      }
      if (status.value() != value_in_body.ToUint32().value()) {
        value_is_same = false;
      }
      break;
    }
    case pbconv::DataPiece::TYPE_UINT64: {
      pb::util::StatusOr<uint32_t> status = value_in_binding.ToUint64();
      if (!status.ok()) {
        return bindingFailureStatus(field_name, "uint64", value_in_binding);
      }
      if (status.value() != value_in_body.ToUint64().value()) {
        value_is_same = false;
      }
      break;
    }
    case pbconv::DataPiece::TYPE_DOUBLE: {
      pb::util::StatusOr<double> status = value_in_binding.ToDouble();
      if (!status.ok()) {
        return bindingFailureStatus(field_name, "double", value_in_binding);
      }
      if (!pb::MathUtil::AlmostEquals<double>(
              status.value(), value_in_body.ToDouble().value())) {
        value_is_same = false;
      }
      break;
    }
    case pbconv::DataPiece::TYPE_FLOAT: {
      pb::util::StatusOr<float> status = value_in_binding.ToFloat();
      if (!status.ok()) {
        return bindingFailureStatus(field_name, "float", value_in_binding);
      }
      if (!pb::MathUtil::AlmostEquals<float>(status.value(),
                                             value_in_body.ToFloat().value())) {
        value_is_same = false;
      }
      break;
    }
    case pbconv::DataPiece::TYPE_BOOL: {
      pb::util::StatusOr<bool> status = value_in_binding.ToBool();
      if (!status.ok()) {
        return bindingFailureStatus(field_name, "bool", value_in_binding);
      }
      if (status.value() != value_in_body.ToBool().value()) {
        value_is_same = false;
      }
      break;
    }
    case pbconv::DataPiece::TYPE_STRING: {
      pb::util::StatusOr<std::string> status = value_in_binding.ToString();
      if (!status.ok()) {
        return bindingFailureStatus(field_name, "string", value_in_binding);
      }
      if (status.value() != value_in_body.ToString().value()) {
        value_is_same = false;
      }
      break;
    }
    case pbconv::DataPiece::TYPE_BYTES: {
      pb::util::StatusOr<std::string> status = value_in_binding.ToBytes();
      if (!status.ok()) {
        return bindingFailureStatus(field_name, "bytes", value_in_binding);
      }
      if (status.value() != value_in_body.ToBytes().value()) {
        value_is_same = false;
      }
      break;
    }
    default:
      break;
  }
  if (!value_is_same) {
    return pb::util::Status(
        pb::util::StatusCode::kInvalidArgument,
        absl::StrFormat("The binding value %s of the field %s is "
                        "conflicting with the value %s in the body.",
                        value_in_binding.ValueAsStringOrDefault(""),
                        std::string(field_name),
                        value_in_body.ValueAsStringOrDefault("")));
  }
  return pb::util::OkStatus();
}

}  // namespace

RequestWeaver::RequestWeaver(std::vector<BindingInfo> bindings,
                             pbconv::ObjectWriter* ow, StatusErrorListener* el,
                             bool report_collisions)
    : root_(),
      current_(),
      ow_(ow),
      non_actionable_depth_(0),
      error_listener_(el),
      report_collisions_(report_collisions) {
  for (const auto& b : bindings) {
    Bind(std::move(b.field_path), std::move(b.value));
  }
}

RequestWeaver* RequestWeaver::StartObject(internal::string_view name) {
  ow_->StartObject(name);
  if (current_.empty()) {
    // The outermost StartObject("");
    current_.push(&root_);
    return this;
  }
  if (non_actionable_depth_ == 0) {
    WeaveInfo* info = current_.top()->FindWeaveMsg(name);
    if (info != nullptr) {
      current_.push(info);
      return this;
    }
  }
  // At this point, we don't match any messages we need to weave into, so
  // we won't need to do any matching until we leave this object.
  ++non_actionable_depth_;
  return this;
}

RequestWeaver* RequestWeaver::EndObject() {
  if (non_actionable_depth_ > 0) {
    --non_actionable_depth_;
  } else {
    WeaveTree(current_.top());
    current_.pop();
  }
  ow_->EndObject();
  return this;
}

RequestWeaver* RequestWeaver::StartList(internal::string_view name) {
  ow_->StartList(name);
  // We don't support weaving inside lists, so we won't need to do any matching
  // until we leave this list.
  ++non_actionable_depth_;
  return this;
}

RequestWeaver* RequestWeaver::EndList() {
  ow_->EndList();
  --non_actionable_depth_;
  return this;
}

RequestWeaver* RequestWeaver::RenderBool(internal::string_view name,
                                         bool value) {
  if (non_actionable_depth_ == 0) {
    pbconv::DataPiece value_in_body = pbconv::DataPiece(value);
    CollisionCheck(name, value_in_body);
  }
  ow_->RenderBool(name, value);
  return this;
}

RequestWeaver* RequestWeaver::RenderInt32(internal::string_view name,
                                          google::protobuf::int32 value) {
  if (non_actionable_depth_ == 0) {
    pbconv::DataPiece value_in_body = pbconv::DataPiece(value);
    CollisionCheck(name, value_in_body);
  }
  ow_->RenderInt32(name, value);
  return this;
}

RequestWeaver* RequestWeaver::RenderUint32(internal::string_view name,
                                           google::protobuf::uint32 value) {
  if (non_actionable_depth_ == 0) {
    pbconv::DataPiece value_in_body = pbconv::DataPiece(value);
    CollisionCheck(name, value_in_body);
  }
  ow_->RenderUint32(name, value);
  return this;
}

RequestWeaver* RequestWeaver::RenderInt64(internal::string_view name,
                                          google::protobuf::int64 value) {
  if (non_actionable_depth_ == 0) {
    pbconv::DataPiece value_in_body = pbconv::DataPiece(value);
    CollisionCheck(name, value_in_body);
  }
  ow_->RenderInt64(name, value);
  return this;
}

RequestWeaver* RequestWeaver::RenderUint64(internal::string_view name,
                                           google::protobuf::uint64 value) {
  if (non_actionable_depth_ == 0) {
    pbconv::DataPiece value_in_body = pbconv::DataPiece(value);
    CollisionCheck(name, value_in_body);
  }
  ow_->RenderUint64(name, value);
  return this;
}

RequestWeaver* RequestWeaver::RenderDouble(internal::string_view name,
                                           double value) {
  if (non_actionable_depth_ == 0) {
    pbconv::DataPiece value_in_body = pbconv::DataPiece(value);
    CollisionCheck(name, value_in_body);
  }
  ow_->RenderDouble(name, value);
  return this;
}

RequestWeaver* RequestWeaver::RenderFloat(internal::string_view name,
                                          float value) {
  if (non_actionable_depth_ == 0) {
    pbconv::DataPiece value_in_body = pbconv::DataPiece(value);
    CollisionCheck(name, value_in_body);
  }
  ow_->RenderFloat(name, value);
  return this;
}

RequestWeaver* RequestWeaver::RenderString(internal::string_view name,
                                           internal::string_view value) {
  if (non_actionable_depth_ == 0) {
    pbconv::DataPiece value_in_body = pbconv::DataPiece(value, true);
    CollisionCheck(name, value_in_body);
  }
  ow_->RenderString(name, value);
  return this;
}

RequestWeaver* RequestWeaver::RenderNull(internal::string_view name) {
  ow_->RenderNull(name);
  return this;
}

RequestWeaver* RequestWeaver::RenderBytes(internal::string_view name,
                                          internal::string_view value) {
  if (non_actionable_depth_ == 0) {
    pbconv::DataPiece value_in_body = pbconv::DataPiece(value, true);
    CollisionCheck(name, value_in_body);
  }
  ow_->RenderBytes(name, value);
  return this;
}

void RequestWeaver::Bind(std::vector<const pb::Field*> field_path,
                         std::string value) {
  WeaveInfo* current = &root_;

  // Find or create the path from the root to the leaf message, where the value
  // should be injected.
  for (size_t i = 0; i < field_path.size() - 1; ++i) {
    current = current->FindOrCreateWeaveMsg(field_path[i]);
  }

  if (!field_path.empty()) {
    current->bindings.emplace_back(field_path.back(), std::move(value));
  }
}

void RequestWeaver::WeaveTree(RequestWeaver::WeaveInfo* info) {
  for (const auto& data : info->bindings) {
    pbconv::ObjectWriter::RenderDataPieceTo(
        pbconv::DataPiece(internal::string_view(data.second), true),
        internal::string_view(data.first->name()), ow_);
  }
  info->bindings.clear();
  for (auto& msg : info->messages) {
    // Enter into the message only if there are bindings or submessages left
    if (!msg.second.bindings.empty() || !msg.second.messages.empty()) {
      ow_->StartObject(msg.first->name());
      WeaveTree(&msg.second);
      ow_->EndObject();
    }
  }
  info->messages.clear();
}

void RequestWeaver::CollisionCheck(internal::string_view name,
                                   const pbconv::DataPiece& value_in_body) {
  if (current_.empty()) return;

  for (auto it = current_.top()->bindings.begin();
       it != current_.top()->bindings.end();) {
    if (name == it->first->name()) {
      if (it->first->cardinality() == pb::Field::CARDINALITY_REPEATED) {
        pbconv::ObjectWriter::RenderDataPieceTo(
            pbconv::DataPiece(internal::string_view(it->second), true), name,
            ow_);
      } else if (report_collisions_) {
        pbconv::DataPiece value_in_binding =
            pbconv::DataPiece(internal::string_view(it->second), true);
        pb::util::Status compare_status =
            isEqual(name, value_in_body, value_in_binding);
        if (!compare_status.ok()) {
          error_listener_->set_status(compare_status);
        }
      }
      it = current_.top()->bindings.erase(it);
      continue;
    }
    ++it;
  }
}

RequestWeaver::WeaveInfo* RequestWeaver::WeaveInfo::FindWeaveMsg(
    const internal::string_view field_name) {
  for (auto& msg : messages) {
    if (field_name == msg.first->name()) {
      return &msg.second;
    }
  }
  return nullptr;
}

RequestWeaver::WeaveInfo* RequestWeaver::WeaveInfo::CreateWeaveMsg(
    const pb::Field* field) {
  messages.emplace_back(field, WeaveInfo());
  return &messages.back().second;
}

RequestWeaver::WeaveInfo* RequestWeaver::WeaveInfo::FindOrCreateWeaveMsg(
    const pb::Field* field) {
  WeaveInfo* found = FindWeaveMsg(field->name());
  return found == nullptr ? CreateWeaveMsg(field) : found;
}

}  // namespace transcoding

}  // namespace grpc
}  // namespace google
