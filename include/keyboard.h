#ifndef YWM_KEYBOARD_H
#define YWM_KEYBOARD_H

#include <wayland-server-core.h>
#include <wlr/types/wlr_keyboard.h>

struct ywm_server;

struct ywm_keyboard {
    struct wl_list       link;      /* ywm_server.keyboards */
    struct ywm_server   *server;
    struct wlr_keyboard *wlr_keyboard;
    struct wl_listener   modifiers;
    struct wl_listener   key;
    struct wl_listener   destroy;
};

#endif /* YWM_KEYBOARD_H */
