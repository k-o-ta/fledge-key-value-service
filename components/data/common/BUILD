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
    name = "message_service",
    srcs = select({
        "//:aws_platform": [
            "msg_svc_aws.cc",
        ],
        "//:local_platform": [
            "msg_svc_local.cc",
        ],
    }) + ["notifier_metadata.h"],
    hdrs = [
        "msg_svc.h",
    ],
    deps = select({
        "//:aws_platform": [
            "//components/errors:aws_error_util",
            "@aws_sdk_cpp//:sns",
            "@aws_sdk_cpp//:sqs",
        ],
        "//conditions:default": [],
    }) + [
        "@com_github_google_glog//:glog",
        "@com_google_absl//absl/random",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
    ],
)

cc_library(
    name = "change_notifier",
    srcs = select({
        "//:aws_platform": [
            "change_notifier_aws.cc",
        ],
        "//:local_platform": [
            "change_notifier_local.cc",
        ],
    }),
    hdrs = [
        "change_notifier.h",
    ],
    deps = select({
        "//:aws_platform": [
            "//components/errors:aws_error_util",
            "@aws_sdk_cpp//:sns",
            "@aws_sdk_cpp//:sqs",
        ],
        "//:local_platform": [
            "@com_google_absl//absl/container:flat_hash_set",
        ],
    }) + [
        ":message_service",
        "//components/util:sleepfor",
        "@com_github_google_glog//:glog",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@google_privacysandbox_servers_common//src/cpp/telemetry:metrics_recorder",
        "@google_privacysandbox_servers_common//src/cpp/telemetry:telemetry_provider",
    ],
)

cc_test(
    name = "change_notifier_test",
    size = "small",
    srcs = select({
        "//:aws_platform": [
            "change_notifier_aws_test.cc",
        ],
        "//:local_platform": [
            "change_notifier_local_test.cc",
        ],
    }),
    deps = select({
        "//:aws_platform": [
            "//components/errors:aws_error_util",
            "//components/util:platform_initializer",
            "@aws_sdk_cpp//:sns",
            "@aws_sdk_cpp//:sqs",
        ],
        "//:local_platform": [
        ],
    }) + [
        ":change_notifier",
        "@com_github_google_glog//:glog",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_googletest//:gtest_main",
        "@google_privacysandbox_servers_common//src/cpp/telemetry:mocks",
    ],
)

cc_library(
    name = "thread_manager",
    srcs = [
        "thread_manager.cc",
    ],
    hdrs = [
        "thread_manager.h",
    ],
    deps = [
        "//components/errors:retry",
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

cc_library(
    name = "mocks",
    testonly = 1,
    hdrs = ["mocks.h"],
    deps = [
        "//components/data/blob_storage:blob_storage_client",
        "//components/data/blob_storage:delta_file_notifier",
        "//components/data/realtime:delta_file_record_change_notifier",
        "//components/data/realtime:realtime_notifier",
        "@com_google_googletest//:gtest",
    ],
)
