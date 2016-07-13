#ifndef WKProxy_h
#define WKProxy_h

#include <WebKit/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

WK_EXPORT WKTypeID WKProxyGetTypeID();

WK_EXPORT WKProxyRef WKProxyCreate(WKStringRef pattern, WKStringRef proxy);

#ifdef __cplusplus
}
#endif

#endif
