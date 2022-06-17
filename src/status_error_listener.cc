#include "grpc_transcoding/status_error_listener.h"

#include <string>

#include "google/protobuf/stubs/stringpiece.h"

namespace google {
namespace grpc {

namespace transcoding {

void StatusErrorListener::InvalidName(
    const ::google::protobuf::util::converter::LocationTrackerInterface& loc,
    internal::string_view unknown_name, internal::string_view message) {
  status_ = ::google::protobuf::util::Status(
      ::google::protobuf::util::StatusCode::kInvalidArgument,
      loc.ToString() + ": " + std::string(message));
}

void StatusErrorListener::InvalidValue(
    const ::google::protobuf::util::converter::LocationTrackerInterface& loc,
    internal::string_view type_name, internal::string_view value) {
  status_ = ::google::protobuf::util::Status(
      ::google::protobuf::util::StatusCode::kInvalidArgument,
      loc.ToString() + ": invalid value " + std::string(value) + " for type " +
          std::string(type_name));
}

void StatusErrorListener::MissingField(
    const ::google::protobuf::util::converter::LocationTrackerInterface& loc,
    internal::string_view missing_name) {
  status_ = ::google::protobuf::util::Status(
      ::google::protobuf::util::StatusCode::kInvalidArgument,
      loc.ToString() + ": missing field " + std::string(missing_name));
}

}  // namespace src::include::grpc_transcoding}  // namespace transcoding

}  // namespace grpc
}  // namespace google