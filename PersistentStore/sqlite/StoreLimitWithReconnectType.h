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

#pragma once

#include <interfaces/IStore2.h>

namespace WPEFramework {
namespace Plugin {
    namespace Sqlite {

        template <typename ACTUALSTORELIMIT>
        class StoreLimitWithReconnectType : public ACTUALSTORELIMIT {
        private:
            StoreLimitWithReconnectType(const StoreLimitWithReconnectType<ACTUALSTORELIMIT>&) = delete;
            StoreLimitWithReconnectType<ACTUALSTORELIMIT>& operator=(const StoreLimitWithReconnectType<ACTUALSTORELIMIT>&) = delete;

        public:
            template <typename... Args>
            StoreLimitWithReconnectType(Args&&... args)
                : ACTUALSTORELIMIT(std::forward<Args>(args)...)
            {
            }
            ~StoreLimitWithReconnectType() override = default;

        public:
            uint32_t SetNamespaceStorageLimit(const Exchange::IStoreInspector::ScopeType scope, const string& ns, const uint32_t size) override
            {
                auto result = ACTUALSTORELIMIT::SetNamespaceStorageLimit(scope, ns, size);
                if (result == Core::ERROR_UNAVAILABLE) {
                    if (ACTUALSTORELIMIT::Open() == Core::ERROR_NONE) {
                        result = ACTUALSTORELIMIT::SetNamespaceStorageLimit(scope, ns, size);
                    }
                }
                if (result == Core::ERROR_UNAVAILABLE) {
                    result = Core::ERROR_GENERAL;
                }
                return result;
            }
        };

    } // namespace Sqlite
} // namespace Plugin
} // namespace WPEFramework
