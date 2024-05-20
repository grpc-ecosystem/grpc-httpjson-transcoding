load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def _non_module_deps_impl(_ctx):
  http_archive(
    name = "com_google_googleapis",
    url = "https://github.com/googleapis/googleapis/archive/1d5522ad1056f16a6d593b8f3038d831e64daeea.tar.gz",
    sha256 = "cd13e547cffaad217c942084fd5ae0985a293d0cce3e788c20796e5e2ea54758",
    strip_prefix = "googleapis-1d5522ad1056f16a6d593b8f3038d831e64daeea",
  )

non_module_deps = module_extension(implementation = _non_module_deps_impl)
