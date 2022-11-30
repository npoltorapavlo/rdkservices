#pragma once

#include "Module.h"

#include "AccessControlList.h"

#include <interfaces/IPersist.h>

namespace WPEFramework {
namespace Plugin {

    class AccessControlListPersist : public Exchange::IPersist {
    public:
        AccessControlListPersist(const AccessControlList& acl, const string& file)
            : _accessControlList(acl)
            , _file(file)
        {
        }
        ~AccessControlListPersist() = default;

        BEGIN_INTERFACE_MAP(AccessControlListPersist)
        INTERFACE_ENTRY(Exchange::IPersist)
        END_INTERFACE_MAP

    private:
        uint32_t Persist() const override
        {
            Core::File file(_file);
            file.Open(false);
            if (!file.IsOpen())
                file.Create();

            return (!file.IsOpen() ? Core::ERROR_WRITE_ERROR : acl.Save(file));
        }

    private:
        const AccessControlList& _accessControlList;
        const string& _file;
    };
}
}
