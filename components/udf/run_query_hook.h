/*
 * Copyright 2023 Google LLC
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

#ifndef COMPONENTS_UDF_RUN_QUERY_HOOK_H_
#define COMPONENTS_UDF_RUN_QUERY_HOOK_H_

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "absl/functional/any_invocable.h"
#include "components/internal_server/run_query_client.h"

namespace kv_server {

// Functor that acts as a wrapper for the internal query client call.
class RunQueryHook {
 public:
  virtual ~RunQueryHook() = default;

  // This is registered with v8 and is exposed to the UDF. Internally, it calls
  // the internal query client.
  virtual std::vector<std::string> operator()(
      std::tuple<std::string>& input) = 0;

  static std::unique_ptr<RunQueryHook> Create(
      absl::AnyInvocable<std::unique_ptr<RunQueryClient>()>
          query_client_supplier);
};

}  // namespace kv_server

#endif  // COMPONENTS_UDF_RUN_QUERY_HOOK_H_
