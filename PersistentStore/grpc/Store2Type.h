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

#include "../Module.h"
#include "secure_storage.grpc.pb.h"
#include <interfaces/IStore2.h>

namespace WPEFramework {
namespace Plugin {
    namespace Grpc {

        template <typename ACTUALSTUB>
        class Store2Type : public Exchange::IStore2, protected ACTUALSTUB {
        private:
            Store2Type(const Store2Type<ACTUALSTUB>&) = delete;
            Store2Type<ACTUALSTUB>& operator=(const Store2Type<ACTUALSTUB>&) = delete;

        public:
            template <typename... Args>
            Store2Type(Args&&... args)
                : Exchange::IStore2()
                , ACTUALSTUB(std::forward<Args>(args)...)
                , _clients()
                , _clientLock()
            {
            }
            ~Store2Type() override = default;

        public:
            // IStore2 methods

            uint32_t Register(Exchange::IStore2::INotification* notification) override
            {
                Core::SafeSyncType<Core::CriticalSection> lock(_clientLock);

                ASSERT(std::find(_clients.begin(), _clients.end(), notification) == _clients.end());

                notification->AddRef();
                _clients.push_back(notification);

                return Core::ERROR_NONE;
            }
            uint32_t Unregister(Exchange::IStore2::INotification* notification) override
            {
                Core::SafeSyncType<Core::CriticalSection> lock(_clientLock);

                std::list<Exchange::IStore2::INotification*>::iterator
                    index(std::find(_clients.begin(), _clients.end(), notification));

                ASSERT(index != _clients.end());

                if (index != _clients.end()) {
                    notification->Release();
                    _clients.erase(index);
                }

                return Core::ERROR_NONE;
            }

            uint32_t SetValue(const ScopeType scope, const string& ns, const string& key, const string& value, const uint32_t ttl) override
            {
                ASSERT(scope == ScopeType::ACCOUNT);

                uint32_t result;

                grpc::ClientContext context;

                distp::gateway::secure_storage::v1::UpdateValueRequest request;
                auto v = new distp::gateway::secure_storage::v1::Value();
                v->set_value(value);
                if (ttl != 0) {
                    auto t = new google::protobuf::Duration();
                    t->set_seconds(ttl);
                    v->set_allocated_ttl(t);
                }
                auto k = new distp::gateway::secure_storage::v1::Key();
                k->set_app_id(ns);
                k->set_key(key);
                k->set_scope(distp::gateway::secure_storage::v1::Scope::SCOPE_ACCOUNT);
                v->set_allocated_key(k);
                request.set_allocated_value(v);

                distp::gateway::secure_storage::v1::UpdateValueResponse response;

                auto status = (*this)->UpdateValue(&context, request, &response);
                if (status.ok()) {
                    OnValueChanged(ns, key, value);
                    result = Core::ERROR_NONE;
                } else {
                    OnError(__FUNCTION__, status);
                    result = Core::ERROR_GENERAL;
                }

                return result;
            }
            uint32_t GetValue(const ScopeType scope, const string& ns, const string& key, string& value, uint32_t& ttl) override
            {
                ASSERT(scope == ScopeType::ACCOUNT);

                uint32_t result;

                grpc::ClientContext context;

                distp::gateway::secure_storage::v1::GetValueRequest request;
                auto k = new distp::gateway::secure_storage::v1::Key();
                k->set_app_id(ns);
                k->set_key(key);
                k->set_scope(distp::gateway::secure_storage::v1::Scope::SCOPE_ACCOUNT);
                request.set_allocated_key(k);

                distp::gateway::secure_storage::v1::GetValueResponse response;

                auto status = (*this)->GetValue(&context, request, &response);
                if (status.ok()) {
                    if (response.has_value()) {
                        auto v = response.value();
                        if (v.has_ttl()) {
                            value = v.value();
                            ttl = v.ttl().seconds();
                            result = Core::ERROR_NONE;
                        } else if (v.has_expire_time()) {
                            value = v.value();
                            ttl = v.expire_time().seconds() - (Core::Time::Now().Ticks() / Core::Time::TicksPerMillisecond / 1000);
                            result = Core::ERROR_NONE;
                        } else {
                            value = v.value();
                            ttl = 0;
                            result = Core::ERROR_NONE;
                        }
                    } else {
                        OnError(__FUNCTION__, status);
                        result = Core::ERROR_GENERAL;
                    }
                } else {
                    OnError(__FUNCTION__, status);
                    result = Core::ERROR_GENERAL;
                }

                return result;
            }
            uint32_t DeleteKey(const ScopeType scope, const string& ns, const string& key) override
            {
                ASSERT(scope == ScopeType::ACCOUNT);

                uint32_t result;

                grpc::ClientContext context;

                distp::gateway::secure_storage::v1::DeleteValueRequest request;
                auto k = new distp::gateway::secure_storage::v1::Key();
                k->set_app_id(ns);
                k->set_key(key);
                k->set_scope(distp::gateway::secure_storage::v1::Scope::SCOPE_ACCOUNT);
                request.set_allocated_key(k);

                distp::gateway::secure_storage::v1::DeleteValueResponse response;

                auto status = (*this)->DeleteValue(&context, request, &response);
                if (status.ok()) {
                    result = Core::ERROR_NONE;
                } else {
                    OnError(__FUNCTION__, status);
                    result = Core::ERROR_GENERAL;
                }

                return result;
            }
            uint32_t DeleteNamespace(const ScopeType scope, const string& ns) override
            {
                ASSERT(scope == ScopeType::ACCOUNT);

                uint32_t result;

                grpc::ClientContext context;

                distp::gateway::secure_storage::v1::DeleteAllValuesRequest request;
                request.set_app_id(ns);
                request.set_scope(distp::gateway::secure_storage::v1::Scope::SCOPE_ACCOUNT);

                distp::gateway::secure_storage::v1::DeleteAllValuesResponse response;

                auto status = (*this)->DeleteAllValues(&context, request, &response);
                if (status.ok()) {
                    result = Core::ERROR_NONE;
                } else {
                    OnError(__FUNCTION__, status);
                    result = Core::ERROR_GENERAL;
                }

                return result;
            }

            BEGIN_INTERFACE_MAP(Store2Type)
            INTERFACE_ENTRY(Exchange::IStore2)
            END_INTERFACE_MAP

        private:
            void OnValueChanged(const string& ns, const string& key, const string& value)
            {
                Core::SafeSyncType<Core::CriticalSection> lock(_clientLock);

                std::list<Exchange::IStore2::INotification*>::iterator
                    index(_clients.begin());

                while (index != _clients.end()) {
                    (*index)->ValueChanged(ScopeType::DEVICE, ns, key, value);
                    index++;
                }
            }
            void OnError(const char* fn, const grpc::Status& status) const
            {
                TRACE(Trace::Error, (_T("%s grpc error %d %s %s"), fn, status.error_code(), status.error_message().c_str(), status.error_details().c_str()));
            }

        private:
            std::list<Exchange::IStore2::INotification*> _clients;
            Core::CriticalSection _clientLock;
        };

    } // namespace Grpc
} // namespace Plugin
} // namespace WPEFramework
