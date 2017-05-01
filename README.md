# grpc-transcoding

grpc-transcoding is a library that supports [transcoding](https://cloud.google.com/endpoints/docs/transcoding) so that HTTP/JSON can be converted to gRPC.

It helps you to provide your APIs in both gRPC and RESTful style at the same time. The code is used in istio [proxy](https://github.com/istio/proxy) to provide HTTP+JSON interface to gRPC service.

The code can be built with the following command

bazel build //...

Tests can be run using

bazel test //...

# Contribution
See [CONTRIBUTING.md](http://github.com/grpc-ecosystem/grpc-transcoding/blob/master/CONTRIBUTING.md).

# License
grpc-transcoding is licensed under the Apache 2.0 license.
See [LICENSE](https://github.com/grpc-ecosystem/grpc-transcoding/blob/master/LICENSE) for more details.

