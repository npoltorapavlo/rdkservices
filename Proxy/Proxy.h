#ifndef PROXY_PROXY_H
#define PROXY_PROXY_H

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

    class Proxy : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    private:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
            {
                Add(_T("mapping"), &Mapping);
            }

        public:
            JsonObject Mapping;
        };

    private:
        Proxy(const Proxy&) = delete;
        Proxy& operator=(const Proxy&) = delete;

    public:
        Proxy() = default;

        BEGIN_INTERFACE_MAP(Proxy)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell*) override {}
        string Information() const override { return {}; }

        //   IWeb methods
        // -------------------------------------------------------------------------------------------------------
        void Inbound(Web::Request&) override {}
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

    private:
        Config _config;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // PROXY_PROXY_H
