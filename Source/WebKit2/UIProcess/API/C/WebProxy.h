#ifndef WebProxy_h
#define WeProxy_h

#include "APIObject.h"
#include <WebCore/Proxy.h>

namespace WebKit {

class WebProxy final : public API::ObjectImpl<API::Object::Type::WebProxy> {
public:
    static Ref<WebProxy> create(const WebCore::Proxy& proxy)
    {
        return adoptRef(*new WebProxy(proxy));
    }

    WebCore::Proxy& proxy() { return m_proxy; }
    const WebCore::Proxy& proxy() const { return m_proxy; }

private:
    WebProxy(const WebCore::Proxy& proxy)
            : m_proxy(proxy) {}

    WebCore::Proxy m_proxy;
};

}

#endif
