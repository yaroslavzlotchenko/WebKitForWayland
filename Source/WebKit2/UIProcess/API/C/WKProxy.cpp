#include "config.h"
#include "WKProxy.h"
#include "WKAPICast.h"
#include "WKString.h"
#include "WebProxy.h"

using namespace WebKit;

WKTypeID WKProxyGetTypeID()
{
    return toAPI(WebKit::WebProxy::APIType);
}

WKProxyRef WKProxyCreate(WKStringRef pattern, WKStringRef proxy)
{
    return toAPI(&WebProxy::create(
            WebCore::Proxy(
                toWTFString(pattern),
                toWTFString(proxy))).leakRef());
}
