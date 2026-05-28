#ifndef YWM_OUTPUT_H
#define YWM_OUTPUT_H

#include <wayland-server-core.h>
#include <wlr/types/wlr_output.h>

struct ywm_server;

struct ywm_output {
    struct wl_list      link;       /* ywm_server.outputs */
    struct ywm_server  *server;
    struct wlr_output  *wlr_output;
    struct wl_listener  frame;
    struct wl_listener  request_state;
    struct wl_listener  destroy;
};

#endif /* YWM_OUTPUT_H */
