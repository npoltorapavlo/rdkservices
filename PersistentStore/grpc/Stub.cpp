/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Stub.h"

#include <grpcpp/create_channel.h>

namespace WPEFramework {
namespace Plugin {
    namespace Grpc {

        Stub::Stub()
        {
            string uri;
            Core::SystemInfo::GetEnvironment(_T("PERSISTENTSTORE_URI"), uri);

            if ((uri.find("localhost") == std::string::npos) && (uri.find("0.0.0.0") == std::string::npos)) {
                string token;
                Core::SystemInfo::GetEnvironment(_T("PERSISTENTSTORE_TOKEN"), token);

                stub_ = distp::gateway::secure_storage::v1::SecureStorageService::NewStub(
                    grpc::CreateChannel(
                        uri,
                        grpc::CompositeChannelCredentials(
                            grpc::SslCredentials(grpc::SslCredentialsOptions()),
                            grpc::AccessTokenCredentials(token))));
            } else {
                // localhost

                stub_ = distp::gateway::secure_storage::v1::SecureStorageService::NewStub(
                    grpc::CreateChannel(
                        uri,
                        grpc::InsecureChannelCredentials()));
            }
        }

    } // namespace Grpc
} // namespace Plugin
} // namespace WPEFramework
