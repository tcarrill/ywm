#include <stdlib.h>
#include <string.h>
#include <cairo/cairo.h>
#include <drm_fourcc.h>

#include <wlr/interfaces/wlr_buffer.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>

#include "view.h"
#include "server.h"

/* ═══════════════════════════════════════════════════════════════════════════
   Minimal wlr_buffer backed by a cairo pixel buffer
   ═══════════════════════════════════════════════════════════════════════════ */

struct frame_buffer {
    struct wlr_buffer base;
    void             *data;
    size_t            stride;
};

static void frame_buf_destroy(struct wlr_buffer *buf) {
    struct frame_buffer *fb = (struct frame_buffer *)buf;
    free(fb->data);
    free(fb);
}

static bool frame_buf_begin_data_ptr_access(struct wlr_buffer *buf,
                                             uint32_t flags,
                                             void **data,
                                             uint32_t *format,
                                             size_t *stride) {
    (void)flags;
    struct frame_buffer *fb = (struct frame_buffer *)buf;
    *data   = fb->data;
    *format = DRM_FORMAT_ARGB8888;
    *stride = fb->stride;
    return true;
}

static void frame_buf_end_data_ptr_access(struct wlr_buffer *buf) {
    (void)buf;
}

static const struct wlr_buffer_impl frame_buf_impl = {
    .destroy               = frame_buf_destroy,
    .begin_data_ptr_access = frame_buf_begin_data_ptr_access,
    .end_data_ptr_access   = frame_buf_end_data_ptr_access,
};

/* ═══════════════════════════════════════════════════════════════════════════
   Geometry helpers
   ═══════════════════════════════════════════════════════════════════════════ */

void view_get_geometry(const struct ywm_view *view, struct wlr_box *box) {
    wlr_xdg_surface_get_geometry(view->xdg_surface, box);
}

int view_client_width(const struct ywm_view *view) {
    struct wlr_box geo = {0};
    wlr_xdg_surface_get_geometry(view->xdg_surface, &geo);
    return geo.width;
}

int view_client_height(const struct ywm_view *view) {
    if (view->shaded) return 0;
    struct wlr_box geo = {0};
    wlr_xdg_surface_get_geometry(view->xdg_surface, &geo);
    return geo.height;
}

static int client_width(const struct ywm_view *view) {
    struct wlr_box geo = {0};
    wlr_xdg_surface_get_geometry(view->xdg_surface, &geo);
    return geo.width;
}

static int client_height(const struct ywm_view *view) {
    if (view->shaded) return 0;
    struct wlr_box geo = {0};
    wlr_xdg_surface_get_geometry(view->xdg_surface, &geo);
    return geo.height;
}

/* ═══════════════════════════════════════════════════════════════════════════
   Hit testing (all coordinates in screen/layout space)

   Frame-window coordinates (fw_x, fw_y) map to screen coords as:
     screen_x = (view->x - BORDER_WIDTH) + fw_x
     screen_y = (view->y - TITLE_BAR_HEIGHT) + fw_y
   ═══════════════════════════════════════════════════════════════════════════ */

bool view_hit_close(const struct ywm_view *view, double lx, double ly) {
    int cbx = view->x - BORDER_WIDTH + CLOSE_BTN_FW_X;
    int cby = view->y - TITLE_BAR_HEIGHT + BTN_TOP_FW_Y;
    return lx >= cbx && lx < cbx + BTN_SIZE &&
           ly >= cby && ly < cby + BTN_SIZE;
}

bool view_hit_shade(const struct ywm_view *view, double lx, double ly) {
    int cw  = client_width(view);
    int sbx = view->x - BORDER_WIDTH + (cw - 8);
    int sby = view->y - TITLE_BAR_HEIGHT + BTN_TOP_FW_Y;
    return lx >= sbx && lx < sbx + BTN_SIZE &&
           ly >= sby && ly < sby + BTN_SIZE;
}

bool view_hit_minimize(const struct ywm_view *view, double lx, double ly) {
    int cw  = client_width(view);
    int mbx = view->x - BORDER_WIDTH + (cw - 24);
    int mby = view->y - TITLE_BAR_HEIGHT + BTN_TOP_FW_Y;
    return lx >= mbx && lx < mbx + BTN_SIZE &&
           ly >= mby && ly < mby + BTN_SIZE;
}

bool view_hit_titlebar(const struct ywm_view *view, double lx, double ly) {
    int cw  = client_width(view);
    int fw  = cw + 2 * BORDER_WIDTH + 2 * X_BORDER_WIDTH;
    int tbx = view->x - BORDER_WIDTH - X_BORDER_WIDTH;
    int tby = view->y - TITLE_BAR_HEIGHT - X_BORDER_WIDTH;
    return lx >= tbx && lx < tbx + fw &&
           ly >= tby && ly < tby + TITLE_BAR_HEIGHT;
}

/*
 * view_hit_resize — mirrors Xlib is_left_frame / is_right_frame /
 * is_bottom_frame / is_lower_left_corner / is_lower_right_corner.
 *
 * Returns a RESIZE_* bitmask for the zone clicked, or 0 if no resize zone.
 * The CORNER_OFFSET logic and priority order match on_motion_notify exactly.
 */
uint32_t view_hit_resize(const struct ywm_view *view, double lx, double ly) {
    if (view->shaded) return 0;
    int cw = client_width(view);
    int ch = client_height(view);

    /* frame-window dimensions (without the outer 1-px X border), matching
     * start_window_geom in the Xlib version */
    int fw = cw + 2 * BORDER_WIDTH;  /* = cw + 8 */
    int fh = ch + TITLE_BAR_HEIGHT + BORDER_WIDTH; /* = ch + 24 */

    /* convert screen → frame-window coords */
    int fx = (int)lx - (view->x - BORDER_WIDTH);
    int fy = (int)ly - (view->y - TITLE_BAR_HEIGHT);

    if (fx < 0 || fx > fw || fy < 0 || fy > fh) return 0;
    /* exclude the titlebar strip — only borders below it are resize zones */
    if (fy <= TITLE_BAR_HEIGHT) return 0;

    bool left   = (fx <= BORDER_WIDTH);
    bool right  = (fx >= fw - BORDER_WIDTH);
    bool bottom = (fy >= fh - BORDER_WIDTH);
    if (!left && !right && !bottom) return 0;

    /* corner priority matches Xlib on_motion_notify order:
     * lower_right > lower_left > left > right > bottom */
    bool lower_right = (bottom && fx >= fw - CORNER_OFFSET)
                    || (right  && fy >= fh - CORNER_OFFSET);
    bool lower_left  = (bottom && fx <= CORNER_OFFSET)
                    || (left   && fy >= fh - CORNER_OFFSET);

    if (lower_right) return RESIZE_RIGHT | RESIZE_BOTTOM;
    if (lower_left)  return RESIZE_LEFT  | RESIZE_BOTTOM;
    if (left)        return RESIZE_LEFT;
    if (right)       return RESIZE_RIGHT;
    return RESIZE_BOTTOM;
}

/* ═══════════════════════════════════════════════════════════════════════════
   draw_button — 13×13 beveled button, pixel-perfect replica of the Xlib
   close/shade button drawing in the original ywm.c.

   Button-local pixel layout (0-indexed, 13×13):
     outer dark  : top row 0, left col 0
     outer light : bottom row 12, right col 12
     black ring  : rows/cols 1 and 11 (1-pixel inset rectangle)
     inner light : top row 2, left col 2
     inner dark  : bottom row 10, right col 10
     face fill   : frame color
   shade-only extra: two horizontal black lines at rows 5 and 7
   minimize-only extra: 4×4 filled black square at upper-left (rows 2-5, cols 2-5)
   btn_type: 0=close, 1=shade, 2=minimize
   ═══════════════════════════════════════════════════════════════════════════ */

static void draw_button(cairo_t *cr, double bx, double by, int btn_type,
                        double lc, double dc) {
    /* face fill — vertical gradient: light at top, face colour at bottom */
    cairo_pattern_t *pat =
        cairo_pattern_create_linear(bx, by, bx, by + BTN_SIZE);
    cairo_pattern_add_color_stop_rgb(pat, 0.0, lc, lc, lc);
    cairo_pattern_add_color_stop_rgb(pat, 1.0, dc, dc, dc);
    cairo_set_source(cr, pat);
    cairo_rectangle(cr, bx, by, BTN_SIZE, BTN_SIZE);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);

    /* outer dark: top + left */
    cairo_set_source_rgb(cr, dc, dc, dc);
    cairo_rectangle(cr, bx,    by,    BTN_SIZE, 1);
    cairo_fill(cr);
    cairo_rectangle(cr, bx,    by,    1, BTN_SIZE);
    cairo_fill(cr);

    /* outer light: bottom + right */
    cairo_set_source_rgb(cr, lc, lc, lc);
    cairo_rectangle(cr, bx,              by + BTN_SIZE - 1, BTN_SIZE, 1);
    cairo_fill(cr);
    cairo_rectangle(cr, bx + BTN_SIZE - 1, by,              1, BTN_SIZE);
    cairo_fill(cr);

    /* black ring (1-pixel inset): top, left, bottom, right */
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_rectangle(cr, bx + 1, by + 1,             BTN_SIZE - 2, 1);
    cairo_fill(cr);
    cairo_rectangle(cr, bx + 1, by + 1,             1, BTN_SIZE - 2);
    cairo_fill(cr);
    cairo_rectangle(cr, bx + 1, by + BTN_SIZE - 2,  BTN_SIZE - 2, 1);
    cairo_fill(cr);
    cairo_rectangle(cr, bx + BTN_SIZE - 2, by + 1,  1, BTN_SIZE - 2);
    cairo_fill(cr);

    /* inner light: top + left (2-pixel inset) */
    cairo_set_source_rgb(cr, lc, lc, lc);
    cairo_rectangle(cr, bx + 2, by + 2, BTN_SIZE - 4, 1);
    cairo_fill(cr);
    cairo_rectangle(cr, bx + 2, by + 2, 1, BTN_SIZE - 4);
    cairo_fill(cr);

    /* inner dark: bottom + right (at row/col 10) */
    cairo_set_source_rgb(cr, dc, dc, dc);
    cairo_rectangle(cr, bx + 2, by + BTN_SIZE - 3, BTN_SIZE - 4, 1);
    cairo_fill(cr);
    cairo_rectangle(cr, bx + BTN_SIZE - 3, by + 2,  1, BTN_SIZE - 4);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0, 0, 0);
    if (btn_type == 1) {
        /* shade: two horizontal lines at rows 5 and 7 */
        cairo_rectangle(cr, bx + 1, by + 5, BTN_SIZE - 2, 1);
        cairo_fill(cr);
        cairo_rectangle(cr, bx + 1, by + 7, BTN_SIZE - 2, 1);
        cairo_fill(cr);
    } else if (btn_type == 2) {
        /* minimize: bottom + right edges of a square whose top and left edges
         * are the button's own inner bevel (row 2, col 2); lines meet at the
         * midpoint of the inner face (col 6, row 6) */
        cairo_rectangle(cr, bx + 2, by + 6, 5, 1);  /* bottom edge */
        cairo_fill(cr);
        cairo_rectangle(cr, bx + 6, by + 2, 1, 5);  /* right edge */
        cairo_fill(cr);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
   render_frame — pixel-perfect Cairo replica of the original Xlib ywm.c
   redraw() + draw_window_titlebar() + draw_close_button() + draw_shade_button()

   Coordinate mapping:
     frame-window coords (fw_x, fw_y)  →  Cairo (fw_x + XBW, fw_y + XBW)
     where XBW = X_BORDER_WIDTH = 1

   Cairo buffer dimensions:
     W = fw + 2*XBW = cw + 2*BW + 2*XBW  (= cw + 10)
     H = fh + 2*XBW
     where fh = TH (shaded) or TH + BW + ch (unshaded)
           fw = cw + 2*BW
   ═══════════════════════════════════════════════════════════════════════════ */

static struct wlr_buffer *render_frame(const char *title,
                                       int cw, int ch,
                                       bool focused, bool shaded,
                                       bool close_hov, bool shade_hov) {
    (void)close_hov;  /* original has no hover visual; reserved for future use */
    (void)shade_hov;

    /* frame-window dimensions (without outer X border) */
    int fw = cw + 2 * BORDER_WIDTH;
    int fh = shaded ? TITLE_BAR_HEIGHT
                    : (ch + TITLE_BAR_HEIGHT + BORDER_WIDTH);

    /* Cairo buffer includes the 1-px outer border on all sides */
    int W = fw + 2 * X_BORDER_WIDTH;
    int H = fh + 2 * X_BORDER_WIDTH;

    if (W <= 0 || H <= 0) return NULL;

    struct frame_buffer *fb = calloc(1, sizeof(*fb));
    fb->stride = (size_t)W * 4;
    fb->data   = calloc(1, fb->stride * (size_t)H);

    cairo_surface_t *cs = cairo_image_surface_create_for_data(
        (unsigned char *)fb->data,
        CAIRO_FORMAT_ARGB32, W, H, (int)fb->stride);
    cairo_t *cr = cairo_create(cs);

    /* ── Color selection ──────────────────────────────────────────────────── */
    double fc, lc, dc;   /* frame, light, dark (all grey; R=G=B) */
    if (focused) {
        fc = 0xaa / 255.0;   /* FOCUSED_FRAME_COLOR  #aaaaaa */
        lc = 0xca / 255.0;   /* FOCUSED_LIGHT_GREY   #cacaca */
        dc = 0x6a / 255.0;   /* FOCUSED_DARK_GREY    #6a6a6a */
    } else {
        fc = 0xcc / 255.0;   /* UNFOCUSED_FRAME_COLOR #cccccc */
        lc = 0xea / 255.0;   /* UNFOCUSED_LIGHT_GREY  #eaeaea */
        dc = 0x76 / 255.0;   /* UNFOCUSED_DARK_GREY   #767676 */
    }

    /* ── 1px outer black X-style border (entire buffer = black, then fill inside) */
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_paint(cr);

    /* Frame background inside the border */
    cairo_set_source_rgb(cr, fc, fc, fc);
    cairo_rectangle(cr, X_BORDER_WIDTH, X_BORDER_WIDTH, fw, fh);
    cairo_fill(cr);

    /*
     * Helper: draw a 1-px horizontal or vertical line expressed in
     * FRAME-WINDOW coordinates.  XDrawLine includes both endpoints, so the
     * length passed here = x2 - x1 + 1  (or y2 - y1 + 1 for vertical).
     *
     * Translates to Cairo by adding X_BORDER_WIDTH to both x and y.
     */
#define XBW X_BORDER_WIDTH
#define SET(g)  cairo_set_source_rgb(cr, (g), (g), (g))
#define HLINE(g, x1, y, len) do {                                     \
        SET(g);                                                         \
        cairo_rectangle(cr, (x1) + XBW, (y) + XBW, (len), 1);         \
        cairo_fill(cr);                                                 \
    } while (0)
#define VLINE(g, x, y1, len) do {                                     \
        SET(g);                                                         \
        cairo_rectangle(cr, (x) + XBW, (y1) + XBW, 1, (len));         \
        cairo_fill(cr);                                                 \
    } while (0)

    /* ── Outer bevel (redraw() lines) ────────────────────────────────────── */

    /* light top:   (0,0)→(fw,0)   — XDrawLine clips to window, so len=fw */
    HLINE(lc, 0, 0, fw);
    /* light left:  (0,1)→(0,fh-1) */
    VLINE(lc, 0, 1, fh - 1);
    /* dark right:  (fw-1,1)→(fw-1,fh) */
    VLINE(dc, fw - 1, 1, fh);
    /* dark bottom: (1,fh-1)→(fw,fh-1) */
    HLINE(dc, 1, fh - 1, fw);

    /* dark titlebar separator: (BW, TH-1)→(fw-BW, TH-1) */
    HLINE(dc, BORDER_WIDTH, TITLE_BAR_HEIGHT - 1,
          fw - 2 * BORDER_WIDTH + 1);

    if (!shaded) {
        /* inner light right:  (fw-BW, TH)→(fw-BW, fh-BW) */
        VLINE(lc, fw - BORDER_WIDTH, TITLE_BAR_HEIGHT,
              fh - TITLE_BAR_HEIGHT - BORDER_WIDTH + 1);

        /* inner light bottom: (BW, fh-BW)→(fw-BW, fh-BW) */
        HLINE(lc, BORDER_WIDTH, fh - BORDER_WIDTH,
              fw - 2 * BORDER_WIDTH + 1);

        /* inner dark left:    (3, TH-1)→(3, fh-BW) */
        VLINE(dc, 3, TITLE_BAR_HEIGHT - 1,
              fh - TITLE_BAR_HEIGHT - BORDER_WIDTH + 1);

        /* ── Corner notches (focused windows only) ──────────────────────── */
        if (focused) {
            /* lower-left horizontal */
            HLINE(dc, 0,  fh - CORNER_OFFSET,     BORDER_WIDTH + 1);
            HLINE(lc, 0,  fh - CORNER_OFFSET + 1, BORDER_WIDTH + 1);
            /* lower-left vertical */
            VLINE(dc, CORNER_OFFSET,     fh - BORDER_WIDTH, BORDER_WIDTH + 1);
            VLINE(lc, CORNER_OFFSET + 1, fh - BORDER_WIDTH, BORDER_WIDTH + 1);
            /* lower-right vertical */
            VLINE(dc, fw - CORNER_OFFSET,     fh - BORDER_WIDTH, BORDER_WIDTH + 1);
            VLINE(lc, fw - CORNER_OFFSET + 1, fh - BORDER_WIDTH, BORDER_WIDTH + 1);
            /* lower-right horizontal */
            HLINE(dc, fw - BORDER_WIDTH, fh - CORNER_OFFSET,     BORDER_WIDTH + 1);
            HLINE(lc, fw - BORDER_WIDTH, fh - CORNER_OFFSET + 1, BORDER_WIDTH + 1);
        }
    }

#undef XBW
#undef SET
#undef HLINE
#undef VLINE

    /* ── Titlebar decorations (draw_window_titlebar()) ───────────────────── */

    cairo_select_font_face(cr, "Arial",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 10.0);   /* matches "Arial-10:bold" in Xft */

    cairo_text_extents_t ext = {0};
    double text_w = 0;
    if (title && title[0]) {
        cairo_text_extents(cr, title, &ext);
        text_w = ext.width;
    }

    /*
     * Title centre: Xlib uses fw/2 - text_width/2 in frame-window coords.
     * Our Cairo surface is W = fw + 2 wide, so centering within W gives
     * (W - text_w) / 2, which equals (fw + 2 - text_w) / 2.
     * Compensate x_bearing so the glyph outline is truly centred.
     */
    double cairo_tx = (W - text_w) / 2.0 - ext.x_bearing;

    /* Baseline: y=14 in frame-window → y=15 in Cairo (add XBW=1) */
    double cairo_ty = 15.0;

    if (focused) {
        /*
         * Pinstripes: 12 alternating light/dark horizontal lines,
         * yoffset=4 in frame-window → Cairo y starting at 5.
         *
         * Even rows  → light, starting at fw_x=19 (Cairo 20)
         * Odd  rows  → dark,  starting at fw_x=20 (Cairo 21)
         *
         * With title: left end = cairo_tx - 10,  right start = cairo_tx + text_w + 7
         * Right end stops 3 px (light) / 4 px (dark) before the minimize button,
         * matching the same gap used between the close button and stripe start.
         * Minimize left edge in Cairo = W - 33; so light = W-36, dark = W-37.
         */
        double rx_end_light = W - 36.0;
        double rx_end_dark  = W - 37.0;

        for (int i = 0; i < 12; i++) {
            double y = (4 + i) + X_BORDER_WIDTH;

            if (i % 2 == 0) {
                /* light row */
                cairo_set_source_rgb(cr, lc, lc, lc);
                double lx1 = 19.0 + X_BORDER_WIDTH;
                double lx2 = text_w > 0 ? cairo_tx - 10.0 : rx_end_light;
                if (lx2 > lx1) {
                    cairo_rectangle(cr, lx1, y, lx2 - lx1, 1);
                    cairo_fill(cr);
                }
                if (text_w > 0 && rx_end_light > cairo_tx + text_w + 7.0) {
                    cairo_rectangle(cr, cairo_tx + text_w + 7.0, y,
                                    rx_end_light - (cairo_tx + text_w + 7.0), 1);
                    cairo_fill(cr);
                }
            } else {
                /* dark row (offset 1 px right relative to light row) */
                cairo_set_source_rgb(cr, dc, dc, dc);
                double lx1 = 20.0 + X_BORDER_WIDTH;
                double lx2 = text_w > 0 ? cairo_tx - 9.0 : rx_end_dark;
                if (lx2 > lx1) {
                    cairo_rectangle(cr, lx1, y, lx2 - lx1, 1);
                    cairo_fill(cr);
                }
                if (text_w > 0 && rx_end_dark > cairo_tx + text_w + 8.0) {
                    cairo_rectangle(cr, cairo_tx + text_w + 8.0, y,
                                    rx_end_dark - (cairo_tx + text_w + 8.0), 1);
                    cairo_fill(cr);
                }
            }
        }
    }

    /* Title text */
    if (focused)
        cairo_set_source_rgba(cr, 0, 0, 0, 1.0);
    else
        cairo_set_source_rgba(cr, 0, 0, 0, 0.5);   /* alpha=32768/65535≈0.5 */

    if (text_w > 0) {
        cairo_move_to(cr, cairo_tx, cairo_ty);
        cairo_show_text(cr, title);
    }

    /* ── Buttons: focused windows only ───────────────────────────────────── */
    if (focused) {
        /* close: fw_x=3, fw_y=4  →  Cairo (4, 5) */
        draw_button(cr, 3 + X_BORDER_WIDTH, 4 + X_BORDER_WIDTH,
                    0, lc, dc);

        /* minimize: fw_x = fw-32, fw_y=4  →  Cairo (fw-31, 5) */
        draw_button(cr, (fw - 32) + X_BORDER_WIDTH, 4 + X_BORDER_WIDTH,
                    2, lc, dc);

        /* shade: fw_x = fw-16 = cw-8, fw_y=4  →  Cairo (cw-7, 5) */
        draw_button(cr, (fw - 16) + X_BORDER_WIDTH, 4 + X_BORDER_WIDTH,
                    1, lc, dc);
    }

    cairo_destroy(cr);
    cairo_surface_destroy(cs);

    wlr_buffer_init(&fb->base, &frame_buf_impl, W, H);
    return &fb->base;
}

/* ═══════════════════════════════════════════════════════════════════════════
   view_init
   ═══════════════════════════════════════════════════════════════════════════ */

void view_init(struct ywm_view *view, struct ywm_server *server,
               struct wlr_xdg_surface *xdg_surface) {
    view->server      = server;
    view->xdg_surface = xdg_surface;
    view->minimized   = false;
    view->icon_slot   = -1;
    view->icon_buf    = NULL;

    view->scene_tree = wlr_scene_tree_create(server->layer_views);
    view->scene_tree->node.data = view;

    struct wlr_scene_tree *surf_tree =
        wlr_scene_xdg_surface_create(view->scene_tree, xdg_surface);

    /*
     * Client origin within scene_tree is at (BW + XBW, TH + XBW) = (5, 21).
     * That matches Xlib: client reparented at (FRAME_BORDER_WIDTH,
     * FRAME_TITLEBAR_HEIGHT) = (4, 20) inside the frame window, which itself
     * sits 1 px inside our Cairo X-border.
     */
    wlr_scene_node_set_position(&surf_tree->node,
                                BORDER_WIDTH + X_BORDER_WIDTH,
                                TITLE_BAR_HEIGHT + X_BORDER_WIDTH);
    xdg_surface->data = surf_tree;

    int count = 0;
    struct ywm_view *v;
    wl_list_for_each(v, &server->views, link) count++;
    view->x = 80 + count * 24;
    view->y = 80 + count * 24;

    /*
     * scene_tree top-left = outer edge of the visual frame (including
     * X_BORDER_WIDTH).  Client at (view->x, view->y) lands at
     * scene_tree_pos + (BW+XBW, TH+XBW) = view->x, view->y.
     */
    wlr_scene_node_set_position(&view->scene_tree->node,
                                view->x - BORDER_WIDTH - X_BORDER_WIDTH,
                                view->y - TITLE_BAR_HEIGHT - X_BORDER_WIDTH);
}

/* ═══════════════════════════════════════════════════════════════════════════
   Internal: render and (re-)attach the full frame buffer
   ═══════════════════════════════════════════════════════════════════════════ */

static void view_update_frame(struct ywm_view *view, bool focused) {
    int cw = client_width(view);
    if (cw <= 0) return;
    int ch = client_height(view);   /* 0 when shaded */

    const char *title = view->xdg_surface->toplevel->title
                      ? view->xdg_surface->toplevel->title
                      : (view->xdg_surface->toplevel->app_id
                         ? view->xdg_surface->toplevel->app_id
                         : "ywm");

    struct wlr_buffer *buf =
        render_frame(title, cw, ch, focused, view->shaded,
                     view->deco.close_hovered, view->deco.shade_hovered);
    if (!buf) return;

    if (!view->deco.frame) {
        view->deco.frame =
            wlr_scene_buffer_create(view->scene_tree, buf);
        wlr_scene_node_set_position(&view->deco.frame->node, 0, 0);
        /* Frame must sit behind the client surface, just as the Xlib frame
         * window is always behind its reparented child window. */
        wlr_scene_node_lower_to_bottom(&view->deco.frame->node);
    } else {
        wlr_scene_buffer_set_buffer(view->deco.frame, buf);
    }

    wlr_buffer_drop(buf);
}

/* ═══════════════════════════════════════════════════════════════════════════
   Public API
   ═══════════════════════════════════════════════════════════════════════════ */

void view_update_title(struct ywm_view *view, bool focused) {
    view_update_frame(view, focused);
}

void view_update_colors(struct ywm_view *view, bool focused) {
    view_update_frame(view, focused);
}

void view_update_decoration(struct ywm_view *view) {
    int cw = client_width(view);
    if (cw <= 0) return;

    struct ywm_view *front = wl_list_empty(&view->server->views) ? NULL :
        wl_container_of(view->server->views.next, front, link);
    bool focused = (front == view);

    view_update_frame(view, focused);

    /* Show/hide client surface when shading */
    struct wlr_scene_tree *surf =
        (struct wlr_scene_tree *)view->xdg_surface->data;
    if (surf)
        wlr_scene_node_set_enabled(&surf->node, !view->shaded);
}

void view_toggle_shade(struct ywm_view *view) {
    if (!view->shaded) {
        struct wlr_box geo = {0};
        wlr_xdg_surface_get_geometry(view->xdg_surface, &geo);
        view->unshaded_height = geo.height;
        view->shaded = true;
    } else {
        view->shaded = false;
    }
    view_update_decoration(view);
}

/* ═══════════════════════════════════════════════════════════════════════════
   Minimize / restore
   ═══════════════════════════════════════════════════════════════════════════ */

static int alloc_icon_slot(struct ywm_server *server) {
    int slot = 0;
    bool used;
    do {
        used = false;
        struct ywm_view *v;
        wl_list_for_each(v, &server->views, link) {
            if (v->minimized && v->icon_slot == slot) {
                used = true;
                slot++;
                break;
            }
        }
    } while (used);
    return slot;
}

/* render_icon — 25×25 raised-bevel tile with truncated window title */
static struct wlr_buffer *render_icon(const struct ywm_view *view) {
    int W = ICON_SIZE, H = ICON_SIZE;

    struct frame_buffer *fb = calloc(1, sizeof(*fb));
    fb->stride = (size_t)W * 4;
    fb->data   = calloc(1, fb->stride * (size_t)H);

    cairo_surface_t *cs = cairo_image_surface_create_for_data(
        (unsigned char *)fb->data, CAIRO_FORMAT_ARGB32, W, H, (int)fb->stride);
    cairo_t *cr = cairo_create(cs);

    double fc = 0xaa / 255.0;
    double lc = 0xca / 255.0;
    double dc = 0x6a / 255.0;

    /* fill */
    cairo_set_source_rgb(cr, fc, fc, fc);
    cairo_rectangle(cr, 0, 0, W, H);
    cairo_fill(cr);

    /* outer bevel: light top+left, dark bottom+right */
    cairo_set_source_rgb(cr, lc, lc, lc);
    cairo_rectangle(cr, 0, 0, W, 1);   cairo_fill(cr);
    cairo_rectangle(cr, 0, 0, 1, H);   cairo_fill(cr);
    cairo_set_source_rgb(cr, dc, dc, dc);
    cairo_rectangle(cr, 0, H - 1, W, 1); cairo_fill(cr);
    cairo_rectangle(cr, W - 1, 0, 1, H); cairo_fill(cr);

    /* black 1px inset ring — bottom and right only */
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_rectangle(cr, 1, H - 2, W - 2, 1); cairo_fill(cr);
    cairo_rectangle(cr, W - 2, 1, 1, H - 2); cairo_fill(cr);

    /* inner bevel: light top+left, dark bottom+right */
    cairo_set_source_rgb(cr, lc, lc, lc);
    cairo_rectangle(cr, 2, 2, W - 4, 1); cairo_fill(cr);
    cairo_rectangle(cr, 2, 2, 1, H - 4); cairo_fill(cr);
    cairo_set_source_rgb(cr, dc, dc, dc);
    cairo_rectangle(cr, 2, H - 3, W - 4, 1); cairo_fill(cr);
    cairo_rectangle(cr, W - 3, 2, 1, H - 4); cairo_fill(cr);

    /* title text — centered, clipped to inner face */
    const char *title = view->xdg_surface->toplevel->title;
    if (title && title[0]) {
        cairo_select_font_face(cr, "Arial",
                               CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 7.0);
        cairo_set_source_rgb(cr, 0, 0, 0);

        cairo_text_extents_t te;
        cairo_text_extents(cr, title, &te);

        double face_w = W - 6;   /* usable width inside bevel */
        double tx;
        if (te.width <= face_w) {
            tx = 3 + (face_w - te.width) / 2.0 - te.x_bearing;
        } else {
            tx = 3 - te.x_bearing;
            cairo_rectangle(cr, 3, 3, face_w, H - 6);
            cairo_clip(cr);
        }
        cairo_move_to(cr, tx, (H - te.height) / 2.0 - te.y_bearing);
        cairo_show_text(cr, title);
    }

    cairo_destroy(cr);
    cairo_surface_destroy(cs);

    wlr_buffer_init(&fb->base, &frame_buf_impl, W, H);
    return &fb->base;
}

void view_toggle_minimize(struct ywm_view *view) {
    struct ywm_server *server = view->server;
    view->minimized  = true;
    view->icon_slot  = alloc_icon_slot(server);

    struct wlr_box lb = {0};
    wlr_output_layout_get_box(server->output_layout, NULL, &lb);
    view->icon_x = lb.x + ICON_PADDING + view->icon_slot * (ICON_SIZE + ICON_PADDING);
    view->icon_y = lb.y + lb.height - ICON_SIZE - ICON_PADDING;

    view->icon_buf = wlr_scene_buffer_create(server->layer_overlay,
                                             render_icon(view));
    wlr_scene_node_set_position(&view->icon_buf->node, view->icon_x, view->icon_y);
    wlr_scene_node_set_enabled(&view->scene_tree->node, false);
}

void view_restore_minimize(struct ywm_view *view) {
    view->minimized = false;
    view->icon_slot = -1;
    wlr_scene_node_set_enabled(&view->scene_tree->node, true);
    wlr_scene_node_destroy(&view->icon_buf->node);
    view->icon_buf = NULL;
}
