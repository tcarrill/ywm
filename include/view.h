#ifndef YWM_VIEW_H
#define YWM_VIEW_H

#include <stdint.h>
#include <stdbool.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_scene.h>

/* ── Decoration geometry (mirrors original Xlib ywm.h) ───────────────────── */
#define TITLE_BAR_HEIGHT  20   /* FRAME_TITLEBAR_HEIGHT */
#define BORDER_WIDTH       4   /* FRAME_BORDER_WIDTH    */
#define X_BORDER_WIDTH     1   /* simulates 1-px Xlib window border */
#define CORNER_OFFSET     20   /* FRAME_CORNER_OFFSET   */
#define BTN_SIZE          13   /* close / shade / minimize button 13×13 px */
#define BTN_TOP_FW_Y       4   /* button top in frame-window coords */
#define CLOSE_BTN_FW_X     3   /* close btn left in frame-window coords */
/* shade btn left in frame-window coords = cw - 6  (computed at runtime) */
/* minimize btn left in frame-window coords = cw - 22 (computed at runtime) */

#define ICON_SIZE         75   /* minimized window icon tile px */
#define ICON_PADDING       4   /* gap around icon tiles px */

#define DBLCLICK_MS      400u

/* Minimum frame dimensions — matches Xlib MIN_WIDTH / MIN_HEIGHT */
#define MIN_FRAME_W     50
#define MIN_FRAME_H     40

/* Resize edge bitmask */
#define RESIZE_LEFT   (1u << 0)
#define RESIZE_RIGHT  (1u << 1)
#define RESIZE_BOTTOM (1u << 2)
#define RESIZE_TOP    (1u << 3)

struct ywm_server;

/* ── Per-window decoration nodes ─────────────────────────────────────────── */
struct ywm_decoration {
    struct wlr_scene_buffer *frame;   /* single Cairo buffer: borders+titlebar */
    bool                     close_hovered;
    bool                     shade_hovered;
    bool                     minimize_hovered;
};

/* ── View (one per XDG toplevel window) ──────────────────────────────────── */
struct ywm_view {
    struct wl_list          link;
    struct ywm_server      *server;
    struct wlr_xdg_surface *xdg_surface;
    struct wlr_scene_tree  *scene_tree;

    struct ywm_decoration   deco;

    int x, y;          /* top-left of the client area in layout space */

    bool    csd;             /* client is drawing its own decorations */

    bool    shaded;
    int     unshaded_height;

    bool                     minimized;
    int                      icon_slot;   /* >=0 when minimized, -1 otherwise */
    int                      icon_x, icon_y;
    struct wlr_scene_buffer *icon_buf;

    uint32_t last_click_ms;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener commit;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener request_maximize;
    struct wl_listener set_title;
};

/* ── View API ─────────────────────────────────────────────────────────────── */
void view_init(struct ywm_view *view, struct ywm_server *server,
               struct wlr_xdg_surface *xdg_surface);
void view_update_decoration(struct ywm_view *view);
void view_update_title(struct ywm_view *view, bool focused);
void view_toggle_shade(struct ywm_view *view);
void view_toggle_minimize(struct ywm_view *view);
void view_restore_minimize(struct ywm_view *view);
void view_update_colors(struct ywm_view *view, bool focused);

bool     view_hit_close    (const struct ywm_view *view, double lx, double ly);
bool     view_hit_shade    (const struct ywm_view *view, double lx, double ly);
bool     view_hit_minimize (const struct ywm_view *view, double lx, double ly);
bool     view_hit_titlebar (const struct ywm_view *view, double lx, double ly);
uint32_t view_hit_resize   (const struct ywm_view *view, double lx, double ly);

void view_get_geometry(const struct ywm_view *view, struct wlr_box *box);
int  view_client_width (const struct ywm_view *view);
int  view_client_height(const struct ywm_view *view);

#endif /* YWM_VIEW_H */
