// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "aws/core/Aws.h"
#include "components/util/platform_initializer.h"

namespace kv_server {

PlatformInitializer::PlatformInitializer() {
  Aws::SDKOptions options;
  // Handle curl SIGPIPEs:
  // https://sdk.amazonaws.com/cpp/api/LATEST/aws-cpp-sdk-core/html/struct_aws_1_1_http_options.html#ac56ca429444ca99aeb2dce6aec35f017
  options.httpOptions.installSigPipeHandler = true;
  Aws::InitAPI(options);
}

PlatformInitializer::~PlatformInitializer() {
  Aws::SDKOptions options;
  Aws::ShutdownAPI(options);
}

}  // namespace kv_server
