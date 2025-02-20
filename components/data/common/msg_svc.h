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

#ifndef COMPONENTS_DATA_COMMON_MSG_SVC_H_
#define COMPONENTS_DATA_COMMON_MSG_SVC_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "components/data/common/msg_svc.h"
#include "components/data/common/notifier_metadata.h"

namespace kv_server {
class MessageService {
 public:
  virtual ~MessageService() = default;

  virtual bool IsSetupComplete() const = 0;

  // Returns url if `IsSetupComplete` is true, or empty string otherwise.
  virtual const std::string& GetSqsUrl() const = 0;

  // Creates an SQS Queue with 80 character random name starting with `prefix`
  // Queue is subscribed to the `sns_arn`.
  // On failure, `SetupQueue` can be recalled.
  virtual absl::Status SetupQueue() = 0;

  // Resets setup state, allowing `SetupQueue` to be recalled if previously
  // successful.
  virtual void Reset() = 0;

  static absl::StatusOr<std::unique_ptr<MessageService>> Create(
      NotifierMetadata notifier_metadata,
      std::optional<int32_t> shard_num = std::nullopt);
};

}  // namespace kv_server

#endif  // COMPONENTS_DATA_COMMON_MSG_SVC_H_
