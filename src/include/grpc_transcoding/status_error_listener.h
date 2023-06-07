#ifndef GRPC_TRANSCODING_STATUS_ERROR_LISTENER_H_
#define GRPC_TRANSCODING_STATUS_ERROR_LISTENER_H_

#include "google/protobuf/util/converter/error_listener.h"
#include "grpc_transcoding/internal/protobuf_types.h"
#include "absl/status/status.h"

namespace google {
namespace grpc {

namespace transcoding {

// StatusErrorListener converts the error events into a Status
class StatusErrorListener
    : public ::google::protobuf::util::converter::ErrorListener {
 public:
  StatusErrorListener() {}
  virtual ~StatusErrorListener() {}

  absl::Status status() const { return status_; }

  // ErrorListener implementation
  void InvalidName(
      const ::google::protobuf::util::converter::LocationTrackerInterface& loc,
      internal::string_view unknown_name, internal::string_view message);
  void InvalidValue(
      const ::google::protobuf::util::converter::LocationTrackerInterface& loc,
      internal::string_view type_name, internal::string_view value);
  void MissingField(
      const ::google::protobuf::util::converter::LocationTrackerInterface& loc,
      internal::string_view missing_name);

  void set_status(absl::Status status) { status_ = status; }

 private:
  absl::Status status_;

  StatusErrorListener(const StatusErrorListener&) = delete;
  StatusErrorListener& operator=(const StatusErrorListener&) = delete;
};

}  // namespace src::include::grpc_transcoding}  // namespace transcoding

}  // namespace grpc
}  // namespace google
#endif  // GRPC_TRANSCODING_STATUS_ERROR_LISTENER_H_
