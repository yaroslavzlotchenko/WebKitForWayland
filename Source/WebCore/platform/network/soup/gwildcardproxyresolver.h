#ifndef __G_WILDCARD_PROXY_RESOLVER_H__
#define __G_WILDCARD_PROXY_RESOLVER_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define G_TYPE_WILDCARD_PROXY_RESOLVER         (g_wildcard_proxy_resolver_get_type ())
#define G_WILDCARD_PROXY_RESOLVER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), G_TYPE_WILDCARD_PROXY_RESOLVER, GWildcardProxyResolver))
#define G_WILDCARD_PROXY_RESOLVER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), G_TYPE_WILDCARD_PROXY_RESOLVER, GWildcardProxyResolverClass))
#define G_IS_WILDCARD_PROXY_RESOLVER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), G_TYPE_WILDCARD_PROXY_RESOLVER))
#define G_IS_WILDCARD_PROXY_RESOLVER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), G_TYPE_WILDCARD_PROXY_RESOLVER))
#define G_WILDCARD_PROXY_RESOLVER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), G_TYPE_WILDCARD_PROXY_RESOLVER, GWildcardProxyResolverClass))

typedef struct _GWildcardProxyResolver GWildcardProxyResolver;
typedef struct _GWildcardProxyResolverPrivate GWildcardProxyResolverPrivate;
typedef struct _GWildcardProxyResolverClass GWildcardProxyResolverClass;

typedef struct
{
    gchar *pattern;
    gchar *proxy;
} Proxy;

struct _GWildcardProxyResolver
{
    GObject parent_instance;

    GWildcardProxyResolverPrivate *priv;
};

struct _GWildcardProxyResolverClass
{
    GObjectClass parent_class;
};

GType g_wildcard_proxy_resolver_get_type(void);

GProxyResolver *g_wildcard_proxy_resolver_new(const gchar *default_proxy);

void g_wildcard_proxy_resolver_set_default_proxy(
        GWildcardProxyResolver *resolver,
        const gchar *default_proxy);

void g_wildcard_proxy_resolver_set_proxies(
        GWildcardProxyResolver  *resolver,
        GPtrArray *proxies);

G_END_DECLS

#endif /* __G_WILDCARD_PROXY_RESOLVER_H__ */

