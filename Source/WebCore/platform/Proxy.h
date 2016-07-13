#ifndef Proxy_h
#define Proxy_h

#include <wtf/text/WTFString.h>

namespace WebCore {

struct Proxy {
	Proxy() {}

	Proxy(const String& pattern, const String& proxy)
		: pattern(pattern), proxy(proxy) {}

	template<class Encoder> void encode(Encoder&) const;
	template<class Decoder> static bool decode(Decoder&, Proxy&);

	String pattern;
	String proxy;
};

template<class Encoder>
void Proxy::encode(Encoder& encoder) const {
    encoder << pattern;
    encoder << proxy;
}

template<class Decoder>
bool Proxy::decode(Decoder& decoder, Proxy& proxy) {
    if (!decoder.decode(proxy.pattern))
        return false;
    if (!decoder.decode(proxy.proxy))
        return false;

    return true;
}

}

#endif
