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

load("@rules_pkg//pkg:mappings.bzl", "pkg_files")

package(default_visibility = ["//visibility:public"])

otel_version = "0.23.0"

otel_tag = "v" + otel_version

target_file = otel_tag + ".tar.gz"

genrule(
    name = "aws_otel_collector",
    outs = ["aws-otel-collector.rpm"],
    cmd = """
        set -x
        yum install -y wget
        yum install -y rpm-build
        wget https://github.com/aws-observability/aws-otel-collector/archive/refs/tags/%s
        tar xvf %s
        cd aws-otel-collector-%s
        # Remove linting from build targets.
        sed -i 's/build: install-tools lint multimod-verify/build:/g' Makefile
        export HOME=/root
        make package-rpm
        cp build/packages/linux/amd64/aws-otel-collector.rpm ../$@
    """ % (target_file, target_file, otel_version),
)

pkg_files(
    name = "aws_otel_collector_cfg",
    srcs = [
        "otel_collector_config.yaml",
    ],
)
