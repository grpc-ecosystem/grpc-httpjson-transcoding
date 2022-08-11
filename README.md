# grpc-httpjson-transcoding

grpc-httpjson-transcoding is a library that supports
[transcoding](https://github.com/googleapis/googleapis/blob/master/google/api/http.proto)
so that HTTP/JSON can be converted to gRPC.

It helps you to provide your APIs in both gRPC and RESTful style at the same
time. The code is used in istio [proxy](https://github.com/istio/proxy) and
cloud [endpoints](https://cloud.google.com/endpoints/) to provide HTTP+JSON
interface to gRPC service.

[![CI Status](https://oss.gprow.dev/badge.svg?jobs=grpc-transcoder-periodic)](https://testgrid.k8s.io/googleoss-grpc-transcoder#Summary)

## Develop

[Bazel](https://bazel.build/) is used for build and dependency management. The
following commands build and test sources:

```bash
$ bazel build //...
$ bazel test //...
```
## Toolchain

The Bazel build system defaults to using clang 10 to enable reproducible builds.


# Contribution
See [CONTRIBUTING.md](CONTRIBUTING.md).

# License
grpc-httpjson-transcoding is licensed under the Apache 2.0 license. See
[LICENSE](LICENSE) for more details.

