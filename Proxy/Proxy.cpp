#include "Proxy.h"

#include "WebLink.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(Proxy, 1, 0);

    const string Proxy::Initialize(PluginHost::IShell* service) /* override */
    {
        _config.FromString(service->ConfigLine());

        return (string());
    }

    Core::ProxyType<Web::Response> Proxy::Process(const Web::Request& request) /* override */
    {
        Core::ProxyType<Web::Response> result;

        JsonObject::Iterator index = _config.Mapping.Variants();
        while (index.Next() == true) {
            if (request.Path.rfind(index.Label(), 0) == 0) {
                auto value = _config.Mapping[index.Label()].String();

                TRACE(Trace::Information, (_T("%s -> %s"), index.Label(), value.c_str()));

                WebLink link(Core::NodeId(value.c_str()));
                result = link.Process(request);

                break;
            }
        }

        if (result.IsValid() == false) {
            result = PluginHost::IFactories::Instance().Response();

            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = _T("Unsupported request for the [Proxy] service.");
        }

        return result;
    }

} // namespace Plugin
} // namespace WPEFramework
