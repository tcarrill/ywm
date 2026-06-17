#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <linux/input-event-codes.h>
#include <xkbcommon/xkbcommon.h>
#include <cairo/cairo.h>
#include <drm_fourcc.h>
#include <wlr/interfaces/wlr_buffer.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_output.h>

#include "server.h"
#include "menu.h"
#include "view.h"
#include "output.h"
#include "keyboard.h"

static void server_focus_view_soft(struct ywm_server *server,
                                   struct ywm_view *view,
                                   struct wlr_surface *surface);

/* Find the ywm_view whose scene_tree owns the topmost scene node at (lx, ly).
 * Works for both client surfaces and frame decoration buffers. */
static struct ywm_view *view_at_point(struct ywm_server *server,
                                      double lx, double ly) {
    struct wlr_scene_node *node =
        wlr_scene_node_at(&server->scene->tree.node, lx, ly, NULL, NULL);
    if (!node) return NULL;
    struct wlr_scene_tree *tree = node->parent;
    while (tree && !tree->node.data)
        tree = tree->node.parent;
    return tree ? tree->node.data : NULL;
}

/* ═══════════════════════════════════════════════════════════════════════════
   Output callbacks
   ═══════════════════════════════════════════════════════════════════════════ */

static void output_frame(struct wl_listener *listener, void *data) {
    (void)data;
    struct ywm_output *output = wl_container_of(listener, output, frame);
    struct wlr_scene_output *scene_output =
        wlr_scene_get_scene_output(output->server->scene, output->wlr_output);
    if (scene_output)
        wlr_scene_output_commit(scene_output, NULL);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (scene_output)
        wlr_scene_output_send_frame_done(scene_output, &now);
}

static void output_request_state(struct wl_listener *listener, void *data) {
    struct ywm_output *output =
        wl_container_of(listener, output, request_state);
    const struct wlr_output_state *state = data;
    wlr_output_commit_state(output->wlr_output, state);
}

static void output_destroy(struct wl_listener *listener, void *data) {
    (void)data;
    struct ywm_output *output = wl_container_of(listener, output, destroy);
    if (output->bg_tile)
        wlr_scene_node_destroy(&output->bg_tile->node);
    wl_list_remove(&output->frame.link);
    wl_list_remove(&output->request_state.link);
    wl_list_remove(&output->destroy.link);
    wl_list_remove(&output->link);
    free(output);
}

static void create_tiled_background(struct ywm_server *server,
                                    struct ywm_output *output);

/* ═══════════════════════════════════════════════════════════════════════════
   Config hot-reload
   ═══════════════════════════════════════════════════════════════════════════ */

static void apply_config(struct ywm_server *server) {
    bool want_picture = (server->cfg.tile_path[0] != '\0');
    wlr_log(WLR_INFO, "apply_config: want_picture=%d path=%s",
            want_picture, want_picture ? server->cfg.tile_path : "(none)");

    /* Background */
    if (want_picture) {
        if (server->bg_rect) {
            wlr_scene_node_destroy(&server->bg_rect->node);
            server->bg_rect = NULL;
        }
        struct ywm_output *output;
        wl_list_for_each(output, &server->outputs, link) {
            if (output->bg_tile) {
                wlr_scene_node_destroy(&output->bg_tile->node);
                output->bg_tile = NULL;
            }
            create_tiled_background(server, output);
        }
    } else {
        struct ywm_output *output;
        wl_list_for_each(output, &server->outputs, link) {
            if (output->bg_tile) {
                wlr_scene_node_destroy(&output->bg_tile->node);
                output->bg_tile = NULL;
            }
        }
        if (server->bg_rect) {
            wlr_scene_rect_set_color(server->bg_rect, server->cfg.bg_color);
        } else {
            server->bg_rect = wlr_scene_rect_create(
                server->layer_background, 65536, 65536, server->cfg.bg_color);
        }
    }

    /* Menu — update values then re-render all cached buffers immediately */
    for (int i = 0; i < 4; i++)
        server->menu.title_color[i] = server->cfg.menu_title_color[i];
    server->menu.alpha = server->cfg.menu_alpha;
    menu_config_changed(&server->menu);

    /* Window decorations */
    struct ywm_view *view;
    wl_list_for_each(view, &server->views, link)
        view_update_decoration(view);
}

static int config_reload_cb(int fd, uint32_t mask, void *data) {
    (void)mask;
    struct ywm_server *server = data;
    /* Read all pending inotify events; reload only if ywm.ini was affected.
     * Watching the directory (not the file) means the watch survives atomic
     * saves where editors replace the file via a temp-file rename. */
    char buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
    bool should_reload = false;
    ssize_t len;
    while ((len = read(fd, buf, sizeof(buf))) > 0) {
        char *ptr = buf;
        while (ptr < buf + (size_t)len) {
            struct inotify_event *ev = (struct inotify_event *)ptr;
            if (ev->len > 0 && strcmp(ev->name, "ywm.ini") == 0)
                should_reload = true;
            ptr += sizeof(struct inotify_event) + ev->len;
        }
    }
    if (should_reload) {
        config_load(&server->cfg);
        apply_config(server);
        wlr_log(WLR_INFO, "ywm: config reloaded");
    }
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════════
   Tiled background — Cairo buffer backed by a malloc'd pixel array,
   same pattern as frame_buffer (view.c) and menu_buffer (menu.c).
   ═══════════════════════════════════════════════════════════════════════════ */

struct bg_buffer {
    struct wlr_buffer base;
    void             *data;
    size_t            stride;
};

static void bg_buf_destroy(struct wlr_buffer *buf) {
    struct bg_buffer *bb = (struct bg_buffer *)buf;
    free(bb->data);
    free(bb);
}

static bool bg_buf_begin_access(struct wlr_buffer *buf, uint32_t flags,
                                 void **data, uint32_t *format, size_t *stride) {
    (void)flags;
    struct bg_buffer *bb = (struct bg_buffer *)buf;
    *data   = bb->data;
    *format = DRM_FORMAT_ARGB8888;
    *stride = bb->stride;
    return true;
}

static void bg_buf_end_access(struct wlr_buffer *buf) { (void)buf; }

static const struct wlr_buffer_impl bg_buf_impl = {
    .destroy               = bg_buf_destroy,
    .begin_data_ptr_access = bg_buf_begin_access,
    .end_data_ptr_access   = bg_buf_end_access,
};

static void create_tiled_background(struct ywm_server *server,
                                    struct ywm_output *output) {
    cairo_surface_t *img =
        cairo_image_surface_create_from_png(server->cfg.tile_path);
    if (cairo_surface_status(img) != CAIRO_STATUS_SUCCESS) {
        wlr_log(WLR_ERROR, "ywm: failed to load background picture: %s",
                server->cfg.tile_path);
        cairo_surface_destroy(img);
        return;
    }

    int w, h;
    wlr_output_effective_resolution(output->wlr_output, &w, &h);

    int img_w = cairo_image_surface_get_width(img);
    int img_h = cairo_image_surface_get_height(img);

    struct bg_buffer *bb = calloc(1, sizeof(*bb));
    bb->stride = (size_t)w * 4;
    bb->data   = calloc(1, bb->stride * (size_t)h);

    cairo_surface_t *cs = cairo_image_surface_create_for_data(
        (unsigned char *)bb->data, CAIRO_FORMAT_ARGB32, w, h, (int)bb->stride);
    cairo_t *cr = cairo_create(cs);

    if (img_w < w || img_h < h) {
        /* Image smaller than output in at least one dimension — tile */
        cairo_pattern_t *pat = cairo_pattern_create_for_surface(img);
        cairo_pattern_set_extend(pat, CAIRO_EXTEND_REPEAT);
        cairo_set_source(cr, pat);
        cairo_paint(cr);
        cairo_pattern_destroy(pat);
    } else {
        /* Image covers the output — scale to fill */
        cairo_scale(cr, (double)w / img_w, (double)h / img_h);
        cairo_set_source_surface(cr, img, 0, 0);
        cairo_paint(cr);
    }

    cairo_destroy(cr);
    cairo_surface_destroy(cs);
    cairo_surface_destroy(img);

    wlr_buffer_init(&bb->base, &bg_buf_impl, w, h);

    struct wlr_box lb = {0};
    wlr_output_layout_get_box(server->output_layout, output->wlr_output, &lb);

    output->bg_tile = wlr_scene_buffer_create(server->layer_background,
                                              &bb->base);
    wlr_scene_node_set_position(&output->bg_tile->node, lb.x, lb.y);
    wlr_buffer_drop(&bb->base);
}

static void server_new_output(struct wl_listener *listener, void *data) {
    struct ywm_server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = data;

    wlr_output_init_render(wlr_output, server->allocator, server->renderer);

    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);

    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    if (mode)
        wlr_output_state_set_mode(&state, mode);

    wlr_output_commit_state(wlr_output, &state);
    wlr_output_state_finish(&state);

    /* Load xcursor at this output's scale */
    wlr_xcursor_manager_load(server->cursor_mgr, wlr_output->scale);

    struct ywm_output *output = calloc(1, sizeof(*output));
    output->server     = server;
    output->wlr_output = wlr_output;

    output->frame.notify         = output_frame;
    output->request_state.notify = output_request_state;
    output->destroy.notify       = output_destroy;
    wl_signal_add(&wlr_output->events.frame,         &output->frame);
    wl_signal_add(&wlr_output->events.request_state, &output->request_state);
    wl_signal_add(&wlr_output->events.destroy,       &output->destroy);

    wl_list_insert(&server->outputs, &output->link);

    struct wlr_output_layout_output *l_out =
        wlr_output_layout_add_auto(server->output_layout, wlr_output);
    struct wlr_scene_output *scene_out =
        wlr_scene_output_create(server->scene, wlr_output);
    wlr_scene_output_layout_add_output(server->scene_layout, l_out, scene_out);

    if (server->cfg.tile_path[0] != '\0')
        create_tiled_background(server, output);
}

/* ═══════════════════════════════════════════════════════════════════════════
   Keyboard callbacks
   ═══════════════════════════════════════════════════════════════════════════ */

static bool handle_keybinding(struct ywm_server *server, xkb_keysym_t sym) {
    switch (sym) {
    case XKB_KEY_Escape:
        wl_display_terminate(server->display);
        return true;
    default:
        return false;
    }
}

static void keyboard_modifiers(struct wl_listener *listener, void *data) {
    (void)data;
    struct ywm_keyboard *kb = wl_container_of(listener, kb, modifiers);
    wlr_seat_set_keyboard(kb->server->seat, kb->wlr_keyboard);
    wlr_seat_keyboard_notify_modifiers(kb->server->seat,
                                       &kb->wlr_keyboard->modifiers);
}

static void keyboard_key(struct wl_listener *listener, void *data) {
    struct ywm_keyboard *kb = wl_container_of(listener, kb, key);
    struct wlr_keyboard_key_event *event = data;
    struct ywm_server *server = kb->server;

    uint32_t keycode = event->keycode + 8;
    const xkb_keysym_t *syms;
    int nsyms = xkb_state_key_get_syms(kb->wlr_keyboard->xkb_state,
                                        keycode, &syms);

    bool handled = false;
    uint32_t mods = wlr_keyboard_get_modifiers(kb->wlr_keyboard);

    if ((mods & WLR_MODIFIER_ALT) &&
        event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        for (int i = 0; i < nsyms; i++)
            handled = handle_keybinding(server, syms[i]) || handled;
    }

    if (!handled) {
        wlr_seat_set_keyboard(server->seat, kb->wlr_keyboard);
        wlr_seat_keyboard_notify_key(server->seat, event->time_msec,
                                     event->keycode, event->state);
    }
}

static void keyboard_destroy(struct wl_listener *listener, void *data) {
    (void)data;
    struct ywm_keyboard *kb = wl_container_of(listener, kb, destroy);
    wl_list_remove(&kb->modifiers.link);
    wl_list_remove(&kb->key.link);
    wl_list_remove(&kb->destroy.link);
    wl_list_remove(&kb->link);
    free(kb);
}

/* ═══════════════════════════════════════════════════════════════════════════
   Input handling
   ═══════════════════════════════════════════════════════════════════════════ */

static void server_new_keyboard(struct ywm_server *server,
                                struct wlr_input_device *device) {
    struct ywm_keyboard *kb = calloc(1, sizeof(*kb));
    kb->server       = server;
    kb->wlr_keyboard = wlr_keyboard_from_input_device(device);

    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap *km   = xkb_keymap_new_from_names(
        ctx, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
    wlr_keyboard_set_keymap(kb->wlr_keyboard, km);
    xkb_keymap_unref(km);
    xkb_context_unref(ctx);
    wlr_keyboard_set_repeat_info(kb->wlr_keyboard, 25, 600);

    kb->modifiers.notify = keyboard_modifiers;
    kb->key.notify       = keyboard_key;
    kb->destroy.notify   = keyboard_destroy;
    wl_signal_add(&kb->wlr_keyboard->events.modifiers, &kb->modifiers);
    wl_signal_add(&kb->wlr_keyboard->events.key,       &kb->key);
    wl_signal_add(&device->events.destroy,             &kb->destroy);

    wlr_seat_set_keyboard(server->seat, kb->wlr_keyboard);
    wl_list_insert(&server->keyboards, &kb->link);
}

static void server_new_pointer(struct ywm_server *server,
                               struct wlr_input_device *device) {
    wlr_cursor_attach_input_device(server->cursor, device);
}

static void server_new_input(struct wl_listener *listener, void *data) {
    struct ywm_server *server = wl_container_of(listener, server, new_input);
    struct wlr_input_device *device = data;

    switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD:
        server_new_keyboard(server, device);
        break;
    case WLR_INPUT_DEVICE_POINTER:
        server_new_pointer(server, device);
        break;
    default:
        break;
    }

    uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
    if (!wl_list_empty(&server->keyboards))
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    wlr_seat_set_capabilities(server->seat, caps);
}

static void seat_request_cursor(struct wl_listener *listener, void *data) {
    struct ywm_server *server =
        wl_container_of(listener, server, request_cursor);
    struct wlr_seat_pointer_request_set_cursor_event *event = data;
    struct wlr_seat_client *focused =
        server->seat->pointer_state.focused_client;
    if (focused == event->seat_client)
        wlr_cursor_set_surface(server->cursor, event->surface,
                               event->hotspot_x, event->hotspot_y);
}

static void seat_request_set_selection(struct wl_listener *listener,
                                        void *data) {
    struct ywm_server *server =
        wl_container_of(listener, server, request_set_selection);
    struct wlr_seat_request_set_selection_event *event = data;
    wlr_seat_set_selection(server->seat, event->source, event->serial);
}

/* ═══════════════════════════════════════════════════════════════════════════
   Cursor / pointer dispatch
   ═══════════════════════════════════════════════════════════════════════════ */

static const char *cursor_name_for_resize(uint32_t edges) {
    if ((edges & RESIZE_TOP)    && (edges & RESIZE_RIGHT)) return "top_right_corner";
    if ((edges & RESIZE_TOP)    && (edges & RESIZE_LEFT))  return "top_left_corner";
    if ((edges & RESIZE_BOTTOM) && (edges & RESIZE_RIGHT)) return "bottom_right_corner";
    if ((edges & RESIZE_BOTTOM) && (edges & RESIZE_LEFT))  return "bottom_left_corner";
    if (edges & (RESIZE_LEFT | RESIZE_RIGHT))               return "sb_h_double_arrow";
    if (edges & (RESIZE_TOP | RESIZE_BOTTOM))               return "sb_v_double_arrow";
    return "default";
}

static void process_cursor_move(struct ywm_server *server) {
    struct ywm_view *view = server->grabbed_view;
    if (!view) return;

    int new_x = (int)(server->cursor->x - server->grab_x);
    int new_y = (int)(server->cursor->y - server->grab_y);

    int cw = view_client_width(view);
    int ch = view_client_height(view);

    /* Shaded windows have no bottom border — their frame ends flush after the
     * titlebar.  fb_off is the offset from view->y to the outer bottom edge. */
    int fb_off = view->shaded ? X_BORDER_WIDTH : ch + BORDER_WIDTH + X_BORDER_WIDTH;

    /* Window-to-window snapping ------------------------------------------ */
    if (server->cfg.window_snap_buffer > 0) {
        int wsnap = server->cfg.window_snap_buffer;
        int wres  = server->cfg.window_resistance;

        /* Frame outer edges of the moving window at candidate position */
        int fl = new_x - BORDER_WIDTH - X_BORDER_WIDTH;
        int fr = new_x + cw  + BORDER_WIDTH + X_BORDER_WIDTH;
        int ft = new_y - TITLE_BAR_HEIGHT   - X_BORDER_WIDTH;
        int fb = new_y + fb_off;

        struct ywm_view *other;
        wl_list_for_each(other, &server->views, link) {
            if (other == view) continue;

            int ocw    = view_client_width(other);
            int och    = view_client_height(other);
            int ofb_off = other->shaded ? X_BORDER_WIDTH
                                        : och + BORDER_WIDTH + X_BORDER_WIDTH;
            int ofl = other->x - BORDER_WIDTH - X_BORDER_WIDTH;
            int ofr = other->x + ocw + BORDER_WIDTH + X_BORDER_WIDTH;
            int oft = other->y - TITLE_BAR_HEIGHT   - X_BORDER_WIDTH;
            int ofb = other->y + ofb_off;

            /* Overlap predicates mirror Xlib is_above/below/left/right */
            bool v_overlap = (fb > oft) && (ft < ofb);
            bool h_overlap = (fr > ofl) && (fl < ofr);
            bool snapped   = false;

            if (v_overlap) {
                /* Snap right: a's right frame approaches b's left frame */
                if (fr + wsnap >= ofl && fr + wsnap <= ofl + wres) {
                    new_x  = other->x - cw - 2 * (BORDER_WIDTH + X_BORDER_WIDTH);
                    snapped = true;
                /* Snap left: a's left frame approaches b's right frame */
                } else if (fl - wsnap <= ofr && fl - wsnap >= ofr - wres) {
                    new_x  = other->x + ocw + 2 * (BORDER_WIDTH + X_BORDER_WIDTH);
                    snapped = true;
                }
            }
            if (!snapped && h_overlap) {
                /* Snap top: a's top frame approaches b's bottom frame */
                if (ft - wsnap <= ofb && ft - wsnap >= ofb - wres) {
                    new_y   = ofb + TITLE_BAR_HEIGHT + X_BORDER_WIDTH;
                    snapped = true;
                /* Snap bottom: a's bottom frame approaches b's top frame */
                } else if (fb + wsnap >= oft && fb + wsnap <= oft + wres) {
                    new_y   = oft - fb_off;
                    snapped = true;
                }
            }
            if (snapped) break;
        }
    }

    /* Screen-edge snapping ----------------------------------------------- */
    if (server->cfg.edge_snap_buffer > 0) {
        int esnap = server->cfg.edge_snap_buffer;
        int eres  = server->cfg.edge_resistance;

        struct wlr_box lb = {0};
        wlr_output_layout_get_box(server->output_layout, NULL, &lb);
        int sx = lb.x,            sy = lb.y;
        int sw = lb.x + lb.width, sh = lb.y + lb.height;

        /* Recompute edges using the (possibly window-snapped) position */
        int fl = new_x - BORDER_WIDTH - X_BORDER_WIDTH;
        int fr = new_x + cw  + BORDER_WIDTH + X_BORDER_WIDTH;
        int ft = new_y - TITLE_BAR_HEIGHT   - X_BORDER_WIDTH;
        int fb = new_y + fb_off;

        /* Horizontal */
        if (fr + esnap >= sw && fr + esnap <= sw + eres)
            new_x = sw - cw  - BORDER_WIDTH - X_BORDER_WIDTH;
        else if (fl - esnap <= sx && fl - esnap >= sx - eres)
            new_x = sx + BORDER_WIDTH + X_BORDER_WIDTH;

        /* Vertical */
        if (fb + esnap >= sh && fb + esnap <= sh + eres)
            new_y = sh - fb_off;
        else if (ft - esnap <= sy && ft - esnap >= sy - eres)
            new_y = sy + TITLE_BAR_HEIGHT + X_BORDER_WIDTH;
    }

    view->x = new_x;
    view->y = new_y;
    wlr_scene_node_set_position(&view->scene_tree->node,
        view->csd ? view->x : view->x - BORDER_WIDTH - X_BORDER_WIDTH,
        view->csd ? view->y : view->y - TITLE_BAR_HEIGHT - X_BORDER_WIDTH);
}

/*
 * Incremental resize — mirrors the Xlib on_motion_notify resize branch.
 * Uses total-offset-from-grab rather than per-event deltas so it never
 * drifts when the client lags behind configure/ack cycles.
 */
static void process_cursor_resize(struct ywm_server *server) {
    struct ywm_view *view = server->grabbed_view;
    if (!view || view->shaded) return;

    double dx = server->cursor->x - server->grab_x;
    double dy = server->cursor->y - server->grab_y;
    uint32_t edges = server->resize_edges;

    int new_cw = server->grab_width;
    int new_ch = server->grab_height;
    int new_vx = server->grab_vx;
    int new_vy = server->grab_vy;

    if (edges & RESIZE_RIGHT)  new_cw = server->grab_width  + (int)dx;
    if (edges & RESIZE_BOTTOM) new_ch = server->grab_height + (int)dy;
    if (edges & RESIZE_LEFT) {
        new_cw = server->grab_width  - (int)dx;
        new_vx = server->grab_vx    + (int)dx;
    }
    if (edges & RESIZE_TOP) {
        new_ch = server->grab_height - (int)dy;
        new_vy = server->grab_vy    + (int)dy;
    }

    /* enforce minimums against frame dimensions, matching Xlib:
     * if (width > MIN_WIDTH && height > MIN_HEIGHT) XMoveResizeWindow(...) */
    int new_fw = new_cw + 2 * BORDER_WIDTH;
    int new_fh = new_ch + TITLE_BAR_HEIGHT + BORDER_WIDTH;
    if (new_fw <= MIN_FRAME_W || new_fh <= MIN_FRAME_H) return;

    /* for left/top-edge resize: update client position immediately */
    if ((edges & RESIZE_LEFT) || (edges & RESIZE_TOP)) {
        view->x = new_vx;
        view->y = new_vy;
        wlr_scene_node_set_position(&view->scene_tree->node,
            view->csd ? view->x : view->x - BORDER_WIDTH - X_BORDER_WIDTH,
            view->csd ? view->y : view->y - TITLE_BAR_HEIGHT - X_BORDER_WIDTH);
    }

    wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr,
                           cursor_name_for_resize(edges));
    wlr_xdg_toplevel_set_size(view->xdg_surface->toplevel, new_cw, new_ch);
    wlr_xdg_surface_schedule_configure(view->xdg_surface);
}

static void process_cursor_motion(struct ywm_server *server, uint32_t time) {
    if (server->cursor_mode == YWM_CURSOR_MOVE) {
        process_cursor_move(server);
        return;
    }
    if (server->cursor_mode == YWM_CURSOR_RESIZE) {
        process_cursor_resize(server);
        return;
    }

    if (server->menu.visible) {
        menu_motion(&server->menu, server->cursor->x, server->cursor->y);
        wlr_seat_pointer_clear_focus(server->seat);
        return;
    }

    double cx = server->cursor->x, cy = server->cursor->y;

    /* Scan views front-to-back for a resize zone; also update button hovers */
    struct ywm_view *front = wl_list_empty(&server->views) ? NULL :
        wl_container_of(server->views.next, front, link);
    struct ywm_view *view;
    const char *hover_cursor = "default";
    wl_list_for_each(view, &server->views, link) {
        bool was_close = view->deco.close_hovered;
        bool was_shade = view->deco.shade_hovered;
        bool was_min   = view->deco.minimize_hovered;
        bool now_close = view_hit_close(view, cx, cy);
        bool now_shade = view_hit_shade(view, cx, cy);
        bool now_min   = view_hit_minimize(view, cx, cy);
        view->deco.close_hovered    = now_close;
        view->deco.shade_hovered    = now_shade;
        view->deco.minimize_hovered = now_min;
        if (now_close != was_close || now_shade != was_shade || now_min != was_min)
            view_update_title(view, view == front);
    }

    /* Resize cursors only on the focused window */
    if (front && hover_cursor[0] == 'd') {
        uint32_t edges = view_hit_resize(front, cx, cy);
        if (edges)
            hover_cursor = cursor_name_for_resize(edges);
    }

    /* Sloppy focus: give keyboard focus to the topmost window under the cursor.
     * Uses the scene graph's own hit-test so Z-order is always respected,
     * and decorations (frame buffer) are detected as well as client surfaces. */
    if (server->cfg.sloppy_focus) {
        struct ywm_view *hovered = view_at_point(server, cx, cy);
        if (hovered && hovered != front)
            server_focus_view_soft(server, hovered,
                                   hovered->xdg_surface->surface);
    }

    /* Deliver pointer focus to surface under cursor */
    struct wlr_surface *surface = NULL;
    double sx, sy;
    server_view_at(server, cx, cy, &surface, &sx, &sy);

    if (surface) {
        wlr_seat_pointer_notify_enter(server->seat, surface, sx, sy);
        wlr_seat_pointer_notify_motion(server->seat, time, sx, sy);
    } else {
        /* Pointer is over decoration or desktop — we own the cursor shape */
        wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, hover_cursor);
        wlr_seat_pointer_clear_focus(server->seat);
    }
}

static void server_cursor_motion(struct wl_listener *listener, void *data) {
    struct ywm_server *server =
        wl_container_of(listener, server, cursor_motion);
    struct wlr_pointer_motion_event *event = data;
    wlr_cursor_move(server->cursor, &event->pointer->base,
                    event->delta_x, event->delta_y);
    process_cursor_motion(server, event->time_msec);
}

static void server_cursor_motion_absolute(struct wl_listener *listener,
                                           void *data) {
    struct ywm_server *server =
        wl_container_of(listener, server, cursor_motion_absolute);
    struct wlr_pointer_motion_absolute_event *event = data;
    wlr_cursor_warp_absolute(server->cursor, &event->pointer->base,
                             event->x, event->y);
    process_cursor_motion(server, event->time_msec);
}

static void server_cursor_button(struct wl_listener *listener, void *data) {
    struct ywm_server *server =
        wl_container_of(listener, server, cursor_button);
    struct wlr_pointer_button_event *event = data;

    double lx = server->cursor->x;
    double ly = server->cursor->y;

    /* Menu intercepts all presses before clients see them */
    if (event->state == WLR_BUTTON_PRESSED && server->menu.visible) {
        if (menu_hit(&server->menu, lx, ly)) {
            if (event->button == BTN_LEFT)
                menu_activate(&server->menu, lx, ly);
            /* other buttons on menu: do nothing, keep it open */
        } else {
            menu_hide(&server->menu);
        }
        return;
    }

    wlr_seat_pointer_notify_button(server->seat, event->time_msec,
                                   event->button, event->state);

    if (event->state == WLR_BUTTON_RELEASED) {
        server->cursor_mode  = YWM_CURSOR_PASSTHROUGH;
        server->grabbed_view = NULL;
        return;
    }

    struct ywm_view *front_before_click = wl_list_empty(&server->views) ? NULL :
        wl_container_of(server->views.next, front_before_click, link);

    struct wlr_surface *surface = NULL;
    double sx, sy;
    struct ywm_view *view =
        server_view_at(server, lx, ly, &surface, &sx, &sy);

    /* server_view_at only finds buffer (client surface) nodes. If the click
     * landed on a decoration buffer, fall back to coordinate hit-testing. */
    if (!view) {
        struct ywm_view *v;
        wl_list_for_each(v, &server->views, link) {
            if (view_hit_titlebar(v, lx, ly) ||
                view_hit_close(v, lx, ly) ||
                view_hit_shade(v, lx, ly) ||
                view_hit_minimize(v, lx, ly) ||
                view_hit_resize(v, lx, ly)) {
                view = v;
                break;
            }
        }
    }

    /* Right-click on empty desktop: open menu */
    if (event->button == BTN_RIGHT && !view) {
        menu_show(&server->menu, (int)lx, (int)ly);
        return;
    }

    /* Icon tile click: restore minimized window */
    if (!view) {
        struct ywm_view *v;
        wl_list_for_each(v, &server->views, link) {
            if (v->minimized &&
                lx >= v->icon_x && lx < v->icon_x + ICON_SIZE &&
                ly >= v->icon_y && ly < v->icon_y + ICON_SIZE) {
                view_restore_minimize(v);
                server_focus_view(server, v, v->xdg_surface->surface);
                return;
            }
        }
        return;
    }

    server_focus_view(server, view,
                      surface ? surface : view->xdg_surface->surface);

    /* Close button */
    if (view_hit_close(view, lx, ly)) {
        wlr_xdg_toplevel_send_close(view->xdg_surface->toplevel);
        return;
    }

    /* Shade button */
    if (view_hit_shade(view, lx, ly)) {
        view_toggle_shade(view);
        return;
    }

    /* Minimize button */
    if (view_hit_minimize(view, lx, ly)) {
        view_toggle_minimize(view);
        return;
    }

    /* Border resize — only on already-focused windows; a click on an
     * unfocused border just focuses the window. */
    uint32_t resize_edges = view_hit_resize(view, lx, ly);
    if (resize_edges && !view->shaded && view == front_before_click) {
        server_begin_interactive(server, view, YWM_CURSOR_RESIZE, resize_edges);
        return;
    }

    /* Title bar — single or double click */
    if (view_hit_titlebar(view, lx, ly)) {
        uint32_t delta = event->time_msec - view->last_click_ms;
        if (delta < DBLCLICK_MS && delta > 0) {
            view_toggle_shade(view);
            view->last_click_ms = 0;
        } else {
            view->last_click_ms = event->time_msec;
            server_begin_interactive(server, view, YWM_CURSOR_MOVE, 0);
        }
        return;
    }
}

static void server_cursor_axis(struct wl_listener *listener, void *data) {
    struct ywm_server *server =
        wl_container_of(listener, server, cursor_axis);
    struct wlr_pointer_axis_event *event = data;
    wlr_seat_pointer_notify_axis(server->seat, event->time_msec,
                                 event->orientation, event->delta,
                                 event->delta_discrete, event->source);
}

static void server_cursor_frame(struct wl_listener *listener, void *data) {
    (void)data;
    struct ywm_server *server =
        wl_container_of(listener, server, cursor_frame);
    wlr_seat_pointer_notify_frame(server->seat);
}

/* ═══════════════════════════════════════════════════════════════════════════
   XDG decoration — force server-side decorations on every window
   ═══════════════════════════════════════════════════════════════════════════ */

static void xdg_decoration_new(struct wl_listener *listener, void *data) {
    struct ywm_server *server = wl_container_of(listener, server, new_xdg_decoration);
    struct wlr_xdg_toplevel_decoration_v1 *deco = data;

    bool client_side = (deco->requested_mode ==
                        WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);

    wlr_log(WLR_INFO, "xdg_decoration: app_id=%s requested=%s",
            deco->toplevel->app_id ? deco->toplevel->app_id : "(unknown)",
            client_side ? "CLIENT_SIDE" : "SERVER_SIDE");

    wlr_xdg_toplevel_decoration_v1_set_mode(deco,
        client_side ? WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE
                    : WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);

    /* Mark the associated view so it is positioned without decoration offsets */
    struct wlr_xdg_surface *xdg_surface = deco->toplevel->base;
    struct ywm_view *v;
    wl_list_for_each(v, &server->views, link) {
        if (v->xdg_surface == xdg_surface) {
            v->csd = client_side;
            break;
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
   XDG surface / toplevel lifecycle
   ═══════════════════════════════════════════════════════════════════════════ */

static void xdg_surface_map(struct wl_listener *listener, void *data) {
    (void)data;
    struct ywm_view *view = wl_container_of(listener, view, map);

    /* Check app_id against the configured CSD list */
    if (!view->csd) {
        const char *app_id = view->xdg_surface->toplevel->app_id;
        wlr_log(WLR_INFO, "xdg_surface_map: app_id=\"%s\"",
                app_id ? app_id : "(null)");
        if (app_id) {
            const struct ywm_config *cfg = &view->server->cfg;
            for (int i = 0; i < cfg->csd_app_count; i++) {
                if (strcmp(app_id, cfg->csd_apps[i]) == 0) {
                    view->csd = true;
                    break;
                }
            }
        }
    }

    view_update_decoration(view);
    struct ywm_view *first = wl_container_of(view->server->views.next, first, link);
    bool focused = !wl_list_empty(&view->server->views) && first == view;
    view_update_title(view, focused);
    server_focus_view(view->server, view, view->xdg_surface->surface);
}

static void xdg_surface_unmap(struct wl_listener *listener, void *data) {
    (void)data;
    struct ywm_view *view = wl_container_of(listener, view, unmap);
    if (view->server->grabbed_view == view) {
        view->server->grabbed_view = NULL;
        view->server->cursor_mode  = YWM_CURSOR_PASSTHROUGH;
    }
}

static void xdg_surface_destroy(struct wl_listener *listener, void *data) {
    (void)data;
    struct ywm_view *view = wl_container_of(listener, view, destroy);
    if (view->minimized && view->icon_buf)
        wlr_scene_node_destroy(&view->icon_buf->node);
    wl_list_remove(&view->map.link);
    wl_list_remove(&view->unmap.link);
    wl_list_remove(&view->destroy.link);
    wl_list_remove(&view->commit.link);
    wl_list_remove(&view->request_move.link);
    wl_list_remove(&view->request_resize.link);
    wl_list_remove(&view->request_maximize.link);
    wl_list_remove(&view->set_title.link);
    wl_list_remove(&view->link);
    wlr_scene_node_destroy(&view->scene_tree->node);
    free(view);
}

static void xdg_surface_commit(struct wl_listener *listener, void *data) {
    (void)data;
    struct ywm_view *view = wl_container_of(listener, view, commit);
    if (view->xdg_surface->initial_commit) {
        wlr_xdg_toplevel_set_size(view->xdg_surface->toplevel, 0, 0);
        return;
    }
    view_update_decoration(view);
}

static void xdg_toplevel_request_move(struct wl_listener *listener,
                                       void *data) {
    (void)data;
    struct ywm_view *view = wl_container_of(listener, view, request_move);
    server_begin_interactive(view->server, view, YWM_CURSOR_MOVE, 0);
}

static void xdg_toplevel_request_resize(struct wl_listener *listener,
                                         void *data) {
    (void)data;
    struct ywm_view *view = wl_container_of(listener, view, request_resize);
    server_begin_interactive(view->server, view, YWM_CURSOR_RESIZE,
                             RESIZE_RIGHT | RESIZE_BOTTOM);
}

static void xdg_toplevel_request_maximize(struct wl_listener *listener,
                                           void *data) {
    (void)data;
    struct ywm_view *view = wl_container_of(listener, view, request_maximize);
    /* Deny: send configure to satisfy protocol requirement */
    wlr_xdg_surface_schedule_configure(view->xdg_surface);
}

static void xdg_toplevel_set_title(struct wl_listener *listener, void *data) {
    (void)data;
    struct ywm_view *view = wl_container_of(listener, view, set_title);
    struct ywm_view *first = wl_container_of(view->server->views.next, first, link);
    bool focused = !wl_list_empty(&view->server->views) && first == view;
    view_update_title(view, focused);
}

static void server_new_xdg_surface(struct wl_listener *listener, void *data) {
    struct ywm_server *server =
        wl_container_of(listener, server, new_xdg_surface);
    struct wlr_xdg_surface *xdg_surface = data;

    /* Popups are handled by the scene graph automatically */
    if (xdg_surface->role == WLR_XDG_SURFACE_ROLE_POPUP) {
        struct wlr_xdg_surface *parent =
            wlr_xdg_surface_try_from_wlr_surface(xdg_surface->popup->parent);
        if (parent && parent->data) {
            struct wlr_scene_tree *parent_tree = parent->data;
            xdg_surface->data =
                wlr_scene_xdg_surface_create(parent_tree, xdg_surface);
        }
        return;
    }

    /* Toplevel */
    struct ywm_view *view = calloc(1, sizeof(*view));
    view_init(view, server, xdg_surface);

    view->map.notify              = xdg_surface_map;
    view->unmap.notify            = xdg_surface_unmap;
    view->destroy.notify          = xdg_surface_destroy;
    view->commit.notify           = xdg_surface_commit;
    view->request_move.notify     = xdg_toplevel_request_move;
    view->request_resize.notify   = xdg_toplevel_request_resize;
    view->request_maximize.notify = xdg_toplevel_request_maximize;
    view->set_title.notify        = xdg_toplevel_set_title;

    wl_signal_add(&xdg_surface->surface->events.map,    &view->map);
    wl_signal_add(&xdg_surface->surface->events.unmap,  &view->unmap);
    wl_signal_add(&xdg_surface->events.destroy,          &view->destroy);
    wl_signal_add(&xdg_surface->surface->events.commit, &view->commit);
    wl_signal_add(&xdg_surface->toplevel->events.request_move,
                  &view->request_move);
    wl_signal_add(&xdg_surface->toplevel->events.request_resize,
                  &view->request_resize);
    wl_signal_add(&xdg_surface->toplevel->events.request_maximize,
                  &view->request_maximize);
    wl_signal_add(&xdg_surface->toplevel->events.set_title,
                  &view->set_title);

    wl_list_insert(&server->views, &view->link);
}

/* ═══════════════════════════════════════════════════════════════════════════
   Public server methods
   ═══════════════════════════════════════════════════════════════════════════ */

/* Focus without raising — used by sloppy focus (focus-follows-mouse). */
static void server_focus_view_soft(struct ywm_server *server,
                                   struct ywm_view *view,
                                   struct wlr_surface *surface) {
    if (!view) return;

    wl_list_remove(&view->link);
    wl_list_insert(&server->views, &view->link);

    struct ywm_view *v;
    wl_list_for_each(v, &server->views, link)
        view_update_colors(v, v == view);

    struct wlr_keyboard *kb = wlr_seat_get_keyboard(server->seat);
    wlr_seat_keyboard_notify_enter(server->seat, surface,
                                   kb ? kb->keycodes : NULL,
                                   kb ? kb->num_keycodes : 0,
                                   kb ? &kb->modifiers : NULL);
}

void server_focus_view(struct ywm_server *server, struct ywm_view *view,
                       struct wlr_surface *surface) {
    if (!view) return;

    /* Raise to top of scene */
    wlr_scene_node_raise_to_top(&view->scene_tree->node);

    /* Move to front of list */
    wl_list_remove(&view->link);
    wl_list_insert(&server->views, &view->link);

    /* Update focused/unfocused colours */
    struct ywm_view *v;
    wl_list_for_each(v, &server->views, link)
        view_update_colors(v, v == view);

    struct wlr_keyboard *kb = wlr_seat_get_keyboard(server->seat);
    wlr_seat_keyboard_notify_enter(server->seat, surface,
                                   kb ? kb->keycodes : NULL,
                                   kb ? kb->num_keycodes : 0,
                                   kb ? &kb->modifiers : NULL);
}

struct ywm_view *server_view_at(struct ywm_server *server,
                                double lx, double ly,
                                struct wlr_surface **surface,
                                double *sx, double *sy) {
    struct wlr_scene_node *node =
        wlr_scene_node_at(&server->scene->tree.node, lx, ly, sx, sy);
    if (!node || node->type != WLR_SCENE_NODE_BUFFER)
        return NULL;

    struct wlr_scene_buffer *sb = wlr_scene_buffer_from_node(node);
    struct wlr_scene_surface *ss = wlr_scene_surface_try_from_buffer(sb);
    if (!ss) return NULL;
    *surface = ss->surface;

    struct wlr_scene_tree *tree = node->parent;
    while (tree && !tree->node.data)
        tree = tree->node.parent;

    return tree ? tree->node.data : NULL;
}

void server_begin_interactive(struct ywm_server *server,
                              struct ywm_view *view,
                              ywm_cursor_mode mode, uint32_t edges) {
    server->grabbed_view = view;
    server->cursor_mode  = mode;
    if (mode == YWM_CURSOR_MOVE) {
        server->grab_x = server->cursor->x - view->x;
        server->grab_y = server->cursor->y - view->y;
    } else if (mode == YWM_CURSOR_RESIZE) {
        struct wlr_box geo = {0};
        wlr_xdg_surface_get_geometry(view->xdg_surface, &geo);
        server->grab_x       = server->cursor->x;
        server->grab_y       = server->cursor->y;
        server->grab_width   = geo.width;
        server->grab_height  = geo.height;
        server->grab_vx      = view->x;
        server->grab_vy      = view->y;
        server->resize_edges = edges;
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
   Init & run
   ═══════════════════════════════════════════════════════════════════════════ */

bool server_init(struct ywm_server *server) {
    config_load(&server->cfg);

    server->display = wl_display_create();

    /* wlr_backend_autocreate takes wl_display in 0.17.1 */
    server->backend = wlr_backend_autocreate(server->display, NULL);
    if (!server->backend) return false;

    server->renderer = wlr_renderer_autocreate(server->backend);
    wlr_renderer_init_wl_display(server->renderer, server->display);

    server->allocator = wlr_allocator_autocreate(server->backend,
                                                  server->renderer);

    server->compositor = wlr_compositor_create(server->display, 5,
                                               server->renderer);
    wlr_subcompositor_create(server->display);
    wlr_data_device_manager_create(server->display);

    /* Output layout — no display arg in 0.17.1 */
    server->output_layout = wlr_output_layout_create();

    /* Scene graph */
    server->scene = wlr_scene_create();
    server->scene_layout =
        wlr_scene_attach_output_layout(server->scene, server->output_layout);

    server->layer_background = wlr_scene_tree_create(&server->scene->tree);
    server->layer_views      = wlr_scene_tree_create(&server->scene->tree);
    server->layer_overlay    = wlr_scene_tree_create(&server->scene->tree);

    if (server->cfg.tile_path[0] == '\0')
        server->bg_rect = wlr_scene_rect_create(server->layer_background,
                                                65536, 65536, server->cfg.bg_color);

    menu_init(&server->menu, server);
    menu_load(&server->menu);

    wl_list_init(&server->views);
    wl_list_init(&server->outputs);
    wl_list_init(&server->keyboards);

    /* XDG shell — 0.17.1 uses new_surface, not new_toplevel */
    server->xdg_shell = wlr_xdg_shell_create(server->display, 3);
    server->new_xdg_surface.notify = server_new_xdg_surface;
    wl_signal_add(&server->xdg_shell->events.new_surface,
                  &server->new_xdg_surface);

    /* XDG decoration */
    server->xdg_decoration_mgr =
        wlr_xdg_decoration_manager_v1_create(server->display);
    server->new_xdg_decoration.notify = xdg_decoration_new;
    wl_signal_add(&server->xdg_decoration_mgr->events.new_toplevel_decoration,
                  &server->new_xdg_decoration);

    /* Cursor */
    server->cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(server->cursor, server->output_layout);
    server->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
    wlr_xcursor_manager_load(server->cursor_mgr, 1);

    server->cursor_motion.notify          = server_cursor_motion;
    server->cursor_motion_absolute.notify = server_cursor_motion_absolute;
    server->cursor_button.notify          = server_cursor_button;
    server->cursor_axis.notify            = server_cursor_axis;
    server->cursor_frame.notify           = server_cursor_frame;
    wl_signal_add(&server->cursor->events.motion,
                  &server->cursor_motion);
    wl_signal_add(&server->cursor->events.motion_absolute,
                  &server->cursor_motion_absolute);
    wl_signal_add(&server->cursor->events.button,
                  &server->cursor_button);
    wl_signal_add(&server->cursor->events.axis,
                  &server->cursor_axis);
    wl_signal_add(&server->cursor->events.frame,
                  &server->cursor_frame);

    /* Input / seat */
    server->new_input.notify = server_new_input;
    wl_signal_add(&server->backend->events.new_input, &server->new_input);

    server->seat = wlr_seat_create(server->display, "seat0");
    server->request_cursor.notify        = seat_request_cursor;
    server->request_set_selection.notify = seat_request_set_selection;
    wl_signal_add(&server->seat->events.request_set_cursor,
                  &server->request_cursor);
    wl_signal_add(&server->seat->events.request_set_selection,
                  &server->request_set_selection);

    /* Output */
    server->new_output.notify = server_new_output;
    wl_signal_add(&server->backend->events.new_output, &server->new_output);

    /* Config file watch — inotify on the config directory so the watch
     * survives atomic saves (temp-file rename replaces the inode). */
    server->cfg_inotify_fd = -1;
    server->cfg_watch      = NULL;
    const char *home = getenv("HOME");
    if (home) {
        snprintf(server->cfg_path, sizeof(server->cfg_path),
                 "%s/.config/ywm/ywm.ini", home);
        char cfg_dir[512];
        snprintf(cfg_dir, sizeof(cfg_dir), "%s/.config/ywm", home);
        int ifd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
        if (ifd >= 0) {
            inotify_add_watch(ifd, cfg_dir, IN_CLOSE_WRITE | IN_MOVED_TO);
            struct wl_event_loop *loop =
                wl_display_get_event_loop(server->display);
            server->cfg_watch =
                wl_event_loop_add_fd(loop, ifd, WL_EVENT_READABLE,
                                     config_reload_cb, server);
            server->cfg_inotify_fd = ifd;
        }
    }

    return true;
}

void server_run(struct ywm_server *server, const char *startup_cmd) {
    const char *socket = wl_display_add_socket_auto(server->display);
    if (!socket) {
        wlr_backend_destroy(server->backend);
        return;
    }

    if (!wlr_backend_start(server->backend)) {
        wlr_backend_destroy(server->backend);
        wl_display_destroy(server->display);
        return;
    }

    setenv("WAYLAND_DISPLAY", socket, true);
    wlr_log(WLR_INFO, "ywm running on WAYLAND_DISPLAY=%s", socket);

    if (startup_cmd) {
        if (fork() == 0) {
            execl("/bin/sh", "/bin/sh", "-c", startup_cmd, NULL);
            _exit(1);
        }
    }

    wl_display_run(server->display);

    wl_display_destroy_clients(server->display);

    if (server->cfg_watch)
        wl_event_source_remove(server->cfg_watch);
    if (server->cfg_inotify_fd >= 0)
        close(server->cfg_inotify_fd);

    wlr_scene_node_destroy(&server->scene->tree.node);
    wlr_xcursor_manager_destroy(server->cursor_mgr);
    wlr_cursor_destroy(server->cursor);
    wlr_output_layout_destroy(server->output_layout);
    wlr_seat_destroy(server->seat);
    wlr_backend_destroy(server->backend);
    wl_display_destroy(server->display);
}
