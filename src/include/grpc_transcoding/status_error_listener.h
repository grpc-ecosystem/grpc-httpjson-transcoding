#ifndef GRPC_TRANSCODING_STATUS_ERROR_LISTENER_H_
#define GRPC_TRANSCODING_STATUS_ERROR_LISTENER_H_

#include "grpc_transcoding/internal/protobuf_types.h"
#include "google/protobuf/util/internal/error_listener.h"

namespace google {
namespace grpc {

namespace transcoding {

// StatusErrorListener converts the error events into a Status
class StatusErrorListener : public ::google::protobuf::util::converter::ErrorListener  {
 public:
  StatusErrorListener() {}
  virtual ~StatusErrorListener() {}

  ::google::protobuf::util::Status status() const { return status_; }

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

  void set_status(::google::protobuf::util::Status status) { status_ = status; }

 private:
  ::google::protobuf::util::Status  status_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(StatusErrorListener);
};

}  // namespace src::include::grpc_transcoding}  // namespace transcoding

}  // namespace grpc
}  // namespace google
#endif  // GRPC_TRANSCODING_STATUS_ERROR_LISTENER_H_
