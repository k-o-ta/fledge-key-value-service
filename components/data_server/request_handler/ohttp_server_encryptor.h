// Copyright 2023 Google LLC
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

#ifndef COMPONENTS_DATA_SERVER_REQUEST_HANDLER_OHTTP_SERVER_ENCRYPTOR_H_
#define COMPONENTS_DATA_SERVER_REQUEST_HANDLER_OHTTP_SERVER_ENCRYPTOR_H_

#include <string>
#include <string_view>

#include "absl/status/statusor.h"
#include "absl/strings/escaping.h"
#include "public/constants.h"
#include "quiche/oblivious_http/oblivious_http_gateway.h"

namespace kv_server {

// Handles server side decryption of requests and encryption of responses.
// Not thread safe. Supports serial decryption/encryption per request.
class OhttpServerEncryptor {
 public:
  // Decrypts incoming request.
  // The return value points to a string stored in decrypted_request_, so its
  // lifetime is tied to that object, which lifetime is in turn tied to the
  // instance of OhttpEncryptor.
  absl::StatusOr<absl::string_view> DecryptRequest(
      absl::string_view encrypted_payload);
  // Encrypts outgoing response. Since OHTTP is stateful, this method should be
  // called after DecryptRequest.
  absl::StatusOr<std::string> EncryptResponse(std::string payload);

 private:
  std::optional<quiche::ObliviousHttpGateway> ohttp_gateway_;
  std::optional<quiche::ObliviousHttpRequest> decrypted_request_;

  // X25519 Secret key (private key).
  // https://www.ietf.org/archive/id/draft-ietf-ohai-ohttp-03.html#appendix-A-2
  const std::string test_private_key_ = absl::HexStringToBytes(
      "3c168975674b2fa8e465970b79c8dcf09f1c741626480bd4c6162fc5b6a98e1a");
};

}  // namespace kv_server

#endif  // COMPONENTS_DATA_SERVER_REQUEST_HANDLER_OHTTP_SERVER_ENCRYPTOR_H_
