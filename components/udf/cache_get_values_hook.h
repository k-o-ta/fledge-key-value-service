
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

#ifndef COMPONENTS_UDF_CACHE_GET_VALUES_HOOK_IMPL_H_
#define COMPONENTS_UDF_CACHE_GET_VALUES_HOOK_IMPL_H_

#include <memory>

#include "components/data_server/cache/cache.h"
#include "components/udf/get_values_hook.h"

namespace kv_server {

// Create a GetValuesHook that will be registered with v8 and calls a cache
// directly.
std::unique_ptr<GetValuesHook> NewCacheGetValuesHook(const Cache& cache);

}  // namespace kv_server

#endif  // COMPONENTS_UDF_CACHE_GET_VALUES_HOOK_IMPL_H_
