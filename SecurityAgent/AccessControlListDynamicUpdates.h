#pragma once

#include "Module.h"

#include "AccessControlList.h"

#include <interfaces/IAccessControlListDynamicUpdates.h>

namespace WPEFramework {
namespace Plugin {

    class AccessControlListDynamicUpdates : public Exchange::IAccessControlListDynamicUpdates {
    public:
        AccessControlListDynamicUpdates(AccessControlList& acl)
            : _accessControlList(acl)
        {
        }
        ~AccessControlListDynamicUpdates() = default;

        BEGIN_INTERFACE_MAP(AccessControlListDynamicUpdates)
        INTERFACE_ENTRY(Exchange::IAccessControlListDynamicUpdates)
        END_INTERFACE_MAP

    private:
        uint32_t Assign(const string& origin, const string& role) override
        {
            JsonObject acl;
            JsonArray assign;
            JsonObject group;
            group["url"] = origin;
            group["role"] = role;
            assign.Add() = group;
            acl["assign"] = assign;
            string text;
            acl.ToString(text);

//            AccessControlList::JSONACL acl;
//            auto& group = acl.Groups.Add();
//            group.URL = origin;
//            group.Role = role;

            return _accessControlList.Load(text);
        }

        uint32_t Revoke(const string& origin, const string& role) override
        {
            // TODO
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);

            auto it = _filterMap.find(role);
            if (it == _filterMap.end()) {
                return (Core::ERROR_UNKNOWN_KEY);
            }

            Core::JSON::ArrayType<JSONACL::Group> newList;
            Core::JSON::ArrayType<JSONACL::Group>::ConstIterator index = _controlList.Groups.Elements();
            while (index.Next() == true) {
                if ((index.Current().URL.Value() != origin)
                    || (index.Current().Role.Value() != role)) {
                    auto& group = newList.Add();
                    group.URL = index.Current().URL.Value();
                    group.Role = index.Current().Role.Value();
                }
            }
            _controlList.Groups = newList;
            _urlMap.remove_if([&](const std::pair<string, Filter&>& x) {
                return ((x.first == origin) && (&x.second == &it->second));
            });
            if (std::find_if(_urlMap.begin(), _urlMap.end(),
                    [&it](const std::pair<string, Filter&>& x) {
                        return (&x.second == &it->second);
                    })
                == _urlMap.end()) {
                _unusedRoles.push_back(role);
            }

            return (Core::ERROR_NONE);
        }

        uint32_t Set(const string& role, const IAccessControlList::mode& mode, IIterator* plugins) override
        {
            AccessControlList::JSONACL acl;
            auto& jsonPlugins = acl.ACL._roles[role];
            jsonPlugins.Default = mode;
            if (plugins != nullptr) {
                while (plugins->Next() == true) {
                    auto& jsonRules = jsonPlugins._plugins[plugins->Callsign()];
                    jsonRules.Default = plugins->Mode();
                    auto* methods = plugins->Methods();
                    if (methods != nullptr) {
                        string str;
                        while (methods->Next(str) == true) {
                            jsonRules.Methods.Add() = str;
                        }
                    }
                }
            }

            return _accessControlList.Load(acl);
        }

        uint32_t Unset(const string& role) override
        {
            // TODO
            Core::SafeSyncType<Core::CriticalSection> lock(_accessControlList._adminLock);

            auto it = _filterMap.find(role);
            if (it == _filterMap.end()) {
                return (Core::ERROR_UNKNOWN_KEY);
            }
            if (std::find_if(_urlMap.begin(), _urlMap.end(),
                    [&it](const std::pair<string, Filter&>& x) {
                        return (&x.second == &it->second);
                    })
                != _urlMap.end()) {
                return (Core::ERROR_INCOMPLETE_CONFIG);
            }

            _controlList.ACL.Remove(role.c_str());
            _filterMap.erase(it);

            return (Core::ERROR_NONE);
        }

    private:
        AccessControlList& _accessControlList;
    };
}
}