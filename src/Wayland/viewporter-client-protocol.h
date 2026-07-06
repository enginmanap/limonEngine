#ifndef LIMONENGINE_VIEWPORTER_CLIENT_PROTOCOL_H
#define LIMONENGINE_VIEWPORTER_CLIENT_PROTOCOL_H

// Minimal subset of the wayland-scanner-generated viewporter client protocol.

#include <wayland-client.h>

#ifdef __cplusplus
extern "C" {
#endif

struct wp_viewport;

#define WP_VIEWPORT_SET_SOURCE 1

static inline void
wp_viewport_set_source(struct wp_viewport *viewport,
                       wl_fixed_t x, wl_fixed_t y,
                       wl_fixed_t width, wl_fixed_t height)
{
    wl_proxy_marshal_flags((struct wl_proxy *) viewport,
        WP_VIEWPORT_SET_SOURCE, NULL,
        wl_proxy_get_version((struct wl_proxy *) viewport),
        0, x, y, width, height);
}

#ifdef __cplusplus
}
#endif

#endif // LIMONENGINE_VIEWPORTER_CLIENT_PROTOCOL_H
