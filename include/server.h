#ifndef YWM_SERVER_H
#define YWM_SERVER_H

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include "config.h"
#include "menu.h"

typedef enum {
    YWM_CURSOR_PASSTHROUGH,
    YWM_CURSOR_MOVE,
    YWM_CURSOR_RESIZE,
} ywm_cursor_mode;

/* Forward declaration */
struct ywm_view;

struct ywm_server {
    struct wl_display              *display;
    struct wlr_backend             *backend;
    struct wlr_renderer            *renderer;
    struct wlr_allocator           *allocator;
    struct wlr_compositor          *compositor;
    struct wlr_scene               *scene;
    struct wlr_scene_output_layout *scene_layout;
    struct wlr_output_layout       *output_layout;
    struct wlr_xdg_shell           *xdg_shell;
    struct wlr_seat                *seat;
    struct wlr_cursor              *cursor;
    struct wlr_xcursor_manager     *cursor_mgr;
    struct wlr_xdg_decoration_manager_v1 *xdg_decoration_mgr;

    struct wl_listener new_output;
    struct wl_listener new_xdg_surface;
    struct wl_listener cursor_motion;
    struct wl_listener cursor_motion_absolute;
    struct wl_listener cursor_button;
    struct wl_listener cursor_axis;
    struct wl_listener cursor_frame;
    struct wl_listener new_input;
    struct wl_listener request_cursor;
    struct wl_listener request_set_selection;
    struct wl_listener new_xdg_decoration;

    /* Views stored as a wl_list; most-recently-focused is at the front */
    struct wl_list views;       /* ywm_view.link */
    struct wl_list outputs;     /* ywm_output.link */
    struct wl_list keyboards;   /* ywm_keyboard.link */

    /* Scene-graph Z layers */
    struct wlr_scene_tree *layer_background;
    struct wlr_scene_tree *layer_views;
    struct wlr_scene_tree *layer_overlay;

    struct ywm_config cfg;
    struct ywm_menu   menu;

    /* Pointer interaction state */
    ywm_cursor_mode     cursor_mode;
    struct ywm_view    *grabbed_view;
    double              grab_x, grab_y;      /* cursor offset (move) or cursor origin (resize) */
    int                 grab_width;          /* client width  at resize-grab start */
    int                 grab_height;         /* client height at resize-grab start */
    int                 grab_vx;             /* view->x at resize-grab start (left-edge) */
    uint32_t            resize_edges;        /* RESIZE_* bitmask */
};

void server_focus_view(struct ywm_server *server, struct ywm_view *view,
                       struct wlr_surface *surface);

struct ywm_view *server_view_at(struct ywm_server *server,
                                double lx, double ly,
                                struct wlr_surface **surface,
                                double *sx, double *sy);

void server_begin_interactive(struct ywm_server *server,
                              struct ywm_view *view,
                              ywm_cursor_mode mode, uint32_t edges);

#endif /* YWM_SERVER_H */
