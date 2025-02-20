# Copyright 2023 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = [
    "//components:__subpackages__",
    "//tools:__subpackages__",
])

cc_library(
    name = "seeking_input_streambuf",
    srcs = ["seeking_input_streambuf.cc"],
    hdrs = ["seeking_input_streambuf.h"],
    deps = [
        "@com_github_google_glog//:glog",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@google_privacysandbox_servers_common//src/cpp/telemetry:metrics_recorder",
        "@google_privacysandbox_servers_common//src/cpp/telemetry:telemetry_provider",
    ],
)

cc_test(
    name = "seeking_input_streambuf_test",
    srcs = [
        "seeking_input_streambuf_test.cc",
    ],
    deps = [
        ":seeking_input_streambuf",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@google_privacysandbox_servers_common//src/cpp/telemetry:mocks",
        "@google_privacysandbox_servers_common//src/cpp/telemetry:telemetry_provider",
    ],
)

cc_library(
    name = "blob_storage_client",
    srcs = select({
        "//:aws_platform": ["blob_storage_client_s3.cc"],
        "//:local_platform": ["blob_storage_client_local.cc"],
    }),
    hdrs = [
        "blob_storage_client.h",
    ],
    deps = select({
        "//:aws_platform": [
            "//components/errors:aws_error_util",
            "@aws_sdk_cpp//:s3",
            "@aws_sdk_cpp//:transfer",
        ],
        "//conditions:default": [],
    }) + [
        ":seeking_input_streambuf",
        "@com_github_google_glog//:glog",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "blob_storage_client_test",
    size = "small",
    srcs = select({
        "//:aws_platform": ["blob_storage_client_s3_test.cc"],
        "//:local_platform": ["blob_storage_client_local_test.cc"],
    }),
    deps = select({
        "//:aws_platform": [
            "//components/errors:aws_error_util",
            "//components/util:platform_initializer",
            "@aws_sdk_cpp//:s3",
            "@aws_sdk_cpp//:transfer",
        ],
        "//conditions:default": [],
    }) + [
        ":blob_storage_client",
        "//components/data/common:mocks",
        "//components/util:sleepfor_mock",
        "//public/data_loading:filename_utils",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/flags:flag",
        "@com_google_googletest//:gtest_main",
        "@google_privacysandbox_servers_common//src/cpp/telemetry:mocks",
    ],
)

cc_library(
    name = "blob_storage_change_notifier",
    srcs = select({
        "//:aws_platform": [
            "blob_storage_change_notifier_s3.cc",
        ],
        "//:local_platform": ["blob_storage_change_notifier_local.cc"],
    }),
    hdrs = [
        "blob_storage_change_notifier.h",
    ],
    deps = [
        "//components/data/common:change_notifier",
        "@com_google_absl//absl/functional:bind_front",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
    ],
)

cc_test(
    name = "blob_storage_change_notifier_test",
    size = "small",
    srcs = select({
        "//:aws_platform": [
            "blob_storage_change_notifier_s3_test.cc",
        ],
        "//:local_platform": ["blob_storage_change_notifier_local_test.cc"],
    }),
    deps = select({
        "//:aws_platform": [
            "//components/errors:aws_error_util",
            "//components/util:platform_initializer",
            "@aws_sdk_cpp//:sqs",
        ],
        "//:local_platform": [
        ],
    }) + [
        ":blob_storage_change_notifier",
        "@com_github_google_glog//:glog",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_googletest//:gtest_main",
        "@google_privacysandbox_servers_common//src/cpp/telemetry:mocks",
    ],
)

cc_library(
    name = "delta_file_notifier",
    srcs = [
        "delta_file_notifier.cc",
    ],
    hdrs = [
        "delta_file_notifier.h",
    ],
    deps = [
        ":blob_storage_change_notifier",
        ":blob_storage_client",
        "//components/data/common:thread_manager",
        "//components/errors:retry",
        "//components/util:sleepfor",
        "//public:constants",
        "//public/data_loading:filename_utils",
        "@com_github_google_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/functional:bind_front",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/synchronization",
        "@google_privacysandbox_servers_common//src/cpp/util:duration",
    ],
)

cc_test(
    name = "delta_file_notifier_test",
    size = "small",
    srcs = [
        "delta_file_notifier_test.cc",
    ],
    deps = [
        ":delta_file_notifier",
        "//components/data/common:mocks",
        "//components/util:sleepfor_mock",
        "//public/data_loading:filename_utils",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_googletest//:gtest_main",
    ],
)
