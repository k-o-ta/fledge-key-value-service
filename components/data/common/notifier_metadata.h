/*
 * Copyright 2022 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef COMPONENTS_DATA_COMMON_NOTIFIER_METADATA_H_
#define COMPONENTS_DATA_COMMON_NOTIFIER_METADATA_H_

#include <filesystem>
#include <string>

#include "components/data/common/msg_svc.h"

// This is a forward declare so that we don't need to import the actual AWS
// SDK when we're running on the local platform.  SQSClient is only used as a
// pointer, and only for when the platform is AWS.
namespace Aws {
namespace SQS {
class SQSClient;
}  // namespace SQS
}  // namespace Aws

namespace kv_server {
class MessageService;

struct CloudNotifierMetadata {
  std::string queue_prefix;
  std::string sns_arn;
  MessageService* queue_manager;

  // If this is set then it will be used instead of a real SQSClient.  The
  // ChangeNotifier takes ownership of this.
  ::Aws::SQS::SQSClient* only_for_testing_sqs_client_ = nullptr;
};
struct LocalNotifierMetadata {
  std::filesystem::path local_directory;
};

using NotifierMetadata =
    std::variant<CloudNotifierMetadata, kv_server::LocalNotifierMetadata>;

inline void SetQueueManager(NotifierMetadata& notifier_metadata,
                            MessageService* queue_manager) {
  switch (notifier_metadata.index()) {
    case 0: {
      std::get<0>(notifier_metadata).queue_manager = queue_manager;
    }
  }
}

}  // namespace kv_server

#endif  // COMPONENTS_DATA_COMMON_NOTIFIER_METADATA_H_
