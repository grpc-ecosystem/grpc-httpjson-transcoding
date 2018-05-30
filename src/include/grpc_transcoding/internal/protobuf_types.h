/* Copyright 2016 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef GRPC_TRANSCODING_INTERNAL_PROTOBUF_TYPES_H_
#define GRPC_TRANSCODING_INTERNAL_PROTOBUF_TYPES_H_

#include "absl/strings/string_view.h"

namespace google {
namespace grpc {
namespace transcoding {
namespace internal {

typedef ::google::protobuf::StringPiece string_view;

inline absl::string_view ToAbslStringView(const string_view& s) {
  return {s.data(), static_cast<absl::string_view::size_type>(s.size())};
}

}  // namespace internal
}  // namespace transcoding
}  // namespace grpc
}  // namespace google

#endif  // GRPC_TRANSCODING_INTERNAL_PROTOBUF_TYPES_H_
