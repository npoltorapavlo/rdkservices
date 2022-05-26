#ifndef PROXY_WEBLINK_H
#define PROXY_WEBLINK_H

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

    class EXTERNAL WebLink
        : public Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, Core::ProxyPoolType<Web::Response>&> {

    private:
        static Core::ProxyPoolType<Web::Response>& ResponseFactory()
        {
            // Static factory is notified when objects unref-ed
            static Core::ProxyPoolType<Web::Response> instance(1);
            return instance;
        }

    private:
        WebLink() = delete;
        WebLink(const WebLink&) = delete;
        WebLink& operator=(const WebLink&) = delete;

        typedef Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, Core::ProxyPoolType<Web::Response>&> BaseClass;

    public:
        WebLink(const Core::NodeId& remote)
            : BaseClass(1, ResponseFactory(), false, remote.AnyInterface(), remote, 256, 1024)
            , _responseSignal(false, true)
            , _openSignal(false, true)
            , _closeSignal(false, true)
        {
        }

        ~WebLink() override
        {
            TRACE(Trace::Information, (_T("WebLink close")));

            Close(0); // 0 mutes the default wait time which is 100 ms

            // Wait for StateChange
            _closeSignal.Lock(Core::infinite);
        }

        Core::ProxyType<Web::Response> Process(const Web::Request& request)
        {
            // One request at a time
            Core::SafeSyncType<Core::CriticalSection> lock(_lock);

            if (Link().IsOpen() == false) {
                Open(0); // 0 mutes the default wait time which is 100 ms

                // Wait for StateChange
                _openSignal.Lock(Core::infinite);
            }

            _responseSignal.ResetEvent();

            // Submit a copy of request
            auto copy = Core::ProxyType<Web::Request>::Create();
            string text;
            request.ToString(text);
            copy->FromString(text);
            Submit(copy);
            TRACE(Trace::Information, (_T("Connection open, request submitted.")));

            // Wait for Received
            _responseSignal.Lock(Core::infinite);

            return _response;
        }

    private:
        //   WebLinkType methods
        // -------------------------------------------------------------------------------------------------------
        void Send(const Core::ProxyType<Web::Request>&) override {}
        void Received(Core::ProxyType<Web::Response>& element) override
        {
            TRACE(Trace::Information, (_T("Response received")));

            _response = element;
            _responseSignal.SetEvent();
        }

        void LinkBody(Core::ProxyType<Web::Response>& element) override
        {
            TRACE(Trace::Information, (_T("Link body %d"), element->ErrorCode));

            element->Body<Web::IBody>(Core::proxy_cast<Web::IBody>(Core::ProxyType<Web::TextBody>::Create()));
        }

        void StateChange() override
        {
            TRACE(Trace::Information, (_T("State change %d %d"), (int)Link().IsOpen(), (int)Link().HasError()));

            if (Link().IsOpen() == true) {
                _openSignal.SetEvent();
            } else {
                _closeSignal.SetEvent();
            }
        }

    private:
        Core::CriticalSection _lock;
        Core::Event _responseSignal;
        Core::Event _openSignal;
        Core::Event _closeSignal;
        Core::ProxyType<Web::Response> _response;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // PROXY_WEBLINK_H
