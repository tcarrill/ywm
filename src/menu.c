#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cairo/cairo.h>
#include <drm_fourcc.h>

#include <wayland-server-core.h>
#include <wlr/interfaces/wlr_buffer.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_scene.h>

#include "menu.h"
#include "server.h"

/* ═══════════════════════════════════════════════════════════════════════════
   Minimal wlr_buffer backed by a cairo pixel buffer (same as view.c)
   ═══════════════════════════════════════════════════════════════════════════ */

struct menu_buffer {
    struct wlr_buffer base;
    void  *data;
    size_t stride;
};

static void mb_destroy(struct wlr_buffer *buf) {
    struct menu_buffer *mb = (struct menu_buffer *)buf;
    free(mb->data);
    free(mb);
}

static bool mb_begin_access(struct wlr_buffer *buf, uint32_t flags,
                             void **data, uint32_t *format, size_t *stride) {
    (void)flags;
    struct menu_buffer *mb = (struct menu_buffer *)buf;
    *data   = mb->data;
    *format = DRM_FORMAT_ARGB8888;
    *stride = mb->stride;
    return true;
}

static void mb_end_access(struct wlr_buffer *buf) { (void)buf; }

static const struct wlr_buffer_impl mb_impl = {
    .destroy               = mb_destroy,
    .begin_data_ptr_access = mb_begin_access,
    .end_data_ptr_access   = mb_end_access,
};

/* ═══════════════════════════════════════════════════════════════════════════
   render_title — pixel-perfect replica of draw_menu_title() from Xlib menu.c

   The title bar is MENU_WIDTH × MENU_TITLE_H = 175×22.  Drawing matches the
   Xlib version exactly; coordinates below are 0-indexed Cairo pixels.

   Width alias:  w = MENU_WIDTH - 1 = 174   (= Xlib's  "width" variable)
   Height alias: h = MENU_TITLE_H - 1 = 21  (= Xlib's "height" variable)

   Fill:  (0,0) size (174×21) with title_color
   Bevel: focused light (#cacaca) top+left, focused dark (#6a6a6a) inner
          bottom+right, black (#000000) outer bottom+right
   ═══════════════════════════════════════════════════════════════════════════ */

static struct wlr_buffer *render_title(const float tc[4]) {
    int W = MENU_WIDTH, H = MENU_TITLE_H;
    int w = W - 1, h = H - 1;   /* Xlib's "width" and "height" */

    struct menu_buffer *mb = calloc(1, sizeof(*mb));
    mb->stride = (size_t)W * 4;
    mb->data   = calloc(1, mb->stride * (size_t)H);

    cairo_surface_t *cs = cairo_image_surface_create_for_data(
        (unsigned char *)mb->data, CAIRO_FORMAT_ARGB32, W, H, (int)mb->stride);
    cairo_t *cr = cairo_create(cs);

    /* XFillRectangle(0,0, w,h) with title color */
    cairo_set_source_rgb(cr, tc[0], tc[1], tc[2]);
    cairo_rectangle(cr, 0, 0, w, h);
    cairo_fill(cr);

    /* focused light grey (#cacaca): top row and left column */
    double lc = 0xca / 255.0;
    cairo_set_source_rgb(cr, lc, lc, lc);
    cairo_rectangle(cr, 0, 0, W, 1);   /* top:  (0,0)→(w,0) — full width */
    cairo_fill(cr);
    cairo_rectangle(cr, 0, 1, 1, h);   /* left: (0,1)→(0,h) */
    cairo_fill(cr);

    /* focused dark grey (#6a6a6a): inner bottom and inner right */
    double dc = 0x6a / 255.0;
    cairo_set_source_rgb(cr, dc, dc, dc);
    cairo_rectangle(cr, 1, h - 1, w, 1);   /* (1,h-1)→(w,h-1) */
    cairo_fill(cr);
    cairo_rectangle(cr, w - 1, 1, 1, h - 1); /* (w-1,1)→(w-1,h-1) */
    cairo_fill(cr);

    /* black: outer right column and outer bottom row */
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_rectangle(cr, w, 0, 1, H);   /* (w,0)→(w,h) */
    cairo_fill(cr);
    cairo_rectangle(cr, 0, h, W, 1);   /* (0,h)→(w,h) */
    cairo_fill(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(cs);
    wlr_buffer_init(&mb->base, &mb_impl, W, H);
    return &mb->base;
}

/* ═══════════════════════════════════════════════════════════════════════════
   render_item — pixel-perfect replica of draw_menu_item() from Xlib menu.c

   Width alias:  w = MENU_WIDTH - 1 = 174
   Height alias: h = MENU_ITEM_H - 1 = 21

   1. Base fill with #aaaaaa (= FOCUSED_FRAME_COLOR = window background)
   2. Unfocused bevel: light (#eaeaea) top+left, dark (#767676) inner
      bottom+right, black outer bottom+right
   3. Inner fill:
        normal : #aaaaaa at (2,2) size (w-3, h-3)  = (171,18)
        flash  : #c8c5e6 at (0,0) size (w-1, h-2)  = (173,19)
   4. Label text: black, "Arial" regular 10, baseline at (7,15)
   ═══════════════════════════════════════════════════════════════════════════ */

static struct wlr_buffer *render_item(const char *label, bool flash) {
    int W = MENU_WIDTH, H = MENU_ITEM_H;
    int w = W - 1, h = H - 1;

    struct menu_buffer *mb = calloc(1, sizeof(*mb));
    mb->stride = (size_t)W * 4;
    mb->data   = calloc(1, mb->stride * (size_t)H);

    cairo_surface_t *cs = cairo_image_surface_create_for_data(
        (unsigned char *)mb->data, CAIRO_FORMAT_ARGB32, W, H, (int)mb->stride);
    cairo_t *cr = cairo_create(cs);

    /* Base fill = window background (#aaaaaa) */
    double fc = 0xaa / 255.0;
    cairo_set_source_rgb(cr, fc, fc, fc);
    cairo_paint(cr);

    /* Unfocused light (#eaeaea): top + left */
    double ulc = 0xea / 255.0;
    cairo_set_source_rgb(cr, ulc, ulc, ulc);
    cairo_rectangle(cr, 0, 0, W, 1);   /* (0,0)→(w,0) */
    cairo_fill(cr);
    cairo_rectangle(cr, 0, 1, 1, h);   /* (0,1)→(0,h) */
    cairo_fill(cr);

    /* Unfocused dark (#767676): inner bottom + inner right */
    double udc = 0x76 / 255.0;
    cairo_set_source_rgb(cr, udc, udc, udc);
    cairo_rectangle(cr, 1, h - 1, w, 1);     /* (1,h-1)→(w,h-1) */
    cairo_fill(cr);
    cairo_rectangle(cr, w - 1, 1, 1, h - 1); /* (w-1,1)→(w-1,h-1) */
    cairo_fill(cr);

    /* Black: outer right + outer bottom */
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_rectangle(cr, w, 0, 1, H);   /* (w,0)→(w,h) */
    cairo_fill(cr);
    cairo_rectangle(cr, 0, h, W, 1);   /* (0,h)→(w,h) */
    cairo_fill(cr);

    /* Inner fill drawn over the bevel, matching Xlib draw order */
    if (flash) {
        /* flash_gc = #c8c5e6 at (0,0) size (w-1, h-2) = (173,19) */
        cairo_set_source_rgb(cr, 0xc8 / 255.0, 0xc5 / 255.0, 0xe6 / 255.0);
        cairo_rectangle(cr, 0, 0, w - 1, h - 2);
        cairo_fill(cr);
    } else {
        /* focused_frame_gc = #aaaaaa at (2,2) size (w-3, h-3) = (171,18) */
        cairo_set_source_rgb(cr, fc, fc, fc);
        cairo_rectangle(cr, 2, 2, w - 3, h - 3);
        cairo_fill(cr);
    }

    /* Label text: "Arial-10:medium" → Arial regular 10, black at (7, 15) */
    if (label && label[0]) {
        cairo_select_font_face(cr, "Arial",
                               CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 10.0);
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_move_to(cr, 7, 15);
        cairo_show_text(cr, label);
    }

    cairo_destroy(cr);
    cairo_surface_destroy(cs);
    wlr_buffer_init(&mb->base, &mb_impl, W, H);
    return &mb->base;
}

/* ═══════════════════════════════════════════════════════════════════════════
   Internal helpers
   ═══════════════════════════════════════════════════════════════════════════ */

static void set_item_buf(struct ywm_menu *menu, int idx, bool flash) {
    struct ywm_menu_item *item = &menu->items[idx];
    struct wlr_buffer *buf = render_item(item->label, flash);
    if (!buf) return;

    if (!item->buf) {
        int y = MENU_TITLE_H + idx * MENU_ITEM_H;
        item->buf = wlr_scene_buffer_create(menu->tree, buf);
        wlr_scene_node_set_position(&item->buf->node, 0, y);
    } else {
        wlr_scene_buffer_set_buffer(item->buf, buf);
    }
    wlr_buffer_drop(buf);
}

static void fork_exec(const char *cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        if (fork() == 0) {
            setsid();
            execl("/bin/sh", "/bin/sh", "-c", cmd, NULL);
            _exit(127);
        }
        _exit(0);
    }
    if (pid > 0) waitpid(pid, NULL, 0);
}

/* ═══════════════════════════════════════════════════════════════════════════
   Flash timer callback — fires every FLASH_MS, alternates item colour 6
   times (matching the Xlib flash_menu() loop), then executes the command.

   flash_count tracks how many ticks have fired:
     count 0 = initial state set by menu_activate (normal)
     count 1 = flash   (i=1, odd in Xlib loop)
     count 2 = normal  (i=2)
     count 3 = flash
     count 4 = normal
     count 5 = flash
     count 6 = execute
   ═══════════════════════════════════════════════════════════════════════════ */

static int flash_timer_cb(void *data) {
    struct ywm_menu *menu = data;

    menu->flash_count++;

    if (menu->flash_count >= 6) {
        /* Restore normal appearance briefly before hiding */
        set_item_buf(menu, menu->flash_item, false);

        char cmd[MENU_CMD_MAX];
        snprintf(cmd, sizeof(cmd), "%s", menu->pending_cmd);
        menu->flash_item  = -1;
        menu->flash_count = 0;
        menu_hide(menu);
        fork_exec(cmd);
        return 0;
    }

    bool fl = (menu->flash_count % 2 != 0);   /* odd ticks = flash */
    set_item_buf(menu, menu->flash_item, fl);
    wl_event_source_timer_update(menu->flash_timer, FLASH_MS);
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════════
   Public API
   ═══════════════════════════════════════════════════════════════════════════ */

void menu_init(struct ywm_menu *menu, struct ywm_server *server) {
    memset(menu, 0, sizeof(*menu));
    menu->hovered    = -1;
    menu->flash_item = -1;

    /* Copy title colour from config so rendering doesn't need the server */
    for (int i = 0; i < 4; i++)
        menu->title_color[i] = server->cfg.menu_title_color[i];

    menu->tree = wlr_scene_tree_create(server->layer_overlay);
    wlr_scene_node_set_enabled(&menu->tree->node, false);

    /* Create the persistent flash timer (initially unarmed) */
    menu->flash_timer = wl_event_loop_add_timer(
        wl_display_get_event_loop(server->display),
        flash_timer_cb, menu);
}

/* ── Load items from ~/.config/ywm/menu.ini ──────────────────────────────── */

void menu_load(struct ywm_menu *menu) {
    const char *home = getenv("HOME");
    if (!home) return;

    char path[512];
    snprintf(path, sizeof(path), "%s/.config/ywm/menu.ini", home);

    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[512];
    while (fgets(line, sizeof(line), f) && menu->count < MENU_MAX_ITEMS) {
        /* Strip trailing newline */
        size_t ln = strlen(line);
        if (ln > 0 && line[ln - 1] == '\n') line[--ln] = '\0';

        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == ';' || *p == '\0') continue;

        /* Support both "Label|command" (Xlib) and "label = command" formats */
        char label[MENU_LABEL_MAX] = {0};
        char cmd[MENU_CMD_MAX]     = {0};

        char *sep = strchr(p, '|');
        if (sep) {
            /* Pipe-delimited (original Xlib format) */
            size_t llen = (size_t)(sep - p);
            if (llen >= MENU_LABEL_MAX) llen = MENU_LABEL_MAX - 1;
            memcpy(label, p, llen);
            snprintf(cmd, MENU_CMD_MAX, "%s", sep + 1);
        } else {
            sep = strchr(p, '=');
            if (!sep) continue;
            /* "key = value" format */
            size_t llen = (size_t)(sep - p);
            if (llen >= MENU_LABEL_MAX) llen = MENU_LABEL_MAX - 1;
            memcpy(label, p, llen);
            char *cv = sep + 1;
            while (*cv == ' ' || *cv == '\t') cv++;
            snprintf(cmd, MENU_CMD_MAX, "%s", cv);
        }

        /* Right-trim label */
        for (int i = (int)strlen(label) - 1;
             i >= 0 && (label[i] == ' ' || label[i] == '\t'); i--)
            label[i] = '\0';
        /* Right-trim command */
        for (int i = (int)strlen(cmd) - 1;
             i >= 0 && (cmd[i] == '\r' || cmd[i] == '\n'); i--)
            cmd[i] = '\0';

        if (!label[0] || !cmd[0]) continue;

        struct ywm_menu_item *item = &menu->items[menu->count++];
        snprintf(item->label,   MENU_LABEL_MAX, "%s", label);
        snprintf(item->command, MENU_CMD_MAX,   "%s", cmd);
    }

    fclose(f);
}

/* ── Show / hide ──────────────────────────────────────────────────────────── */

void menu_show(struct ywm_menu *menu, int x, int y) {
    /* Build title buffer once */
    if (!menu->title_buf) {
        struct wlr_buffer *buf = render_title(menu->title_color);
        if (buf) {
            menu->title_buf = wlr_scene_buffer_create(menu->tree, buf);
            wlr_scene_node_set_position(&menu->title_buf->node, 0, 0);
            wlr_buffer_drop(buf);
        }
    }

    /* Build item buffers once */
    for (int i = 0; i < menu->count; i++)
        set_item_buf(menu, i, false);

    menu->x       = x;
    menu->y       = y;
    menu->hovered = -1;
    wlr_scene_node_set_position(&menu->tree->node, x, y);
    wlr_scene_node_set_enabled(&menu->tree->node, true);
    menu->visible = true;
}

void menu_hide(struct ywm_menu *menu) {
    /* Cancel any running flash animation */
    if (menu->flash_item >= 0) {
        wl_event_source_timer_update(menu->flash_timer, 0);
        set_item_buf(menu, menu->flash_item, false);
        menu->flash_item  = -1;
        menu->flash_count = 0;
    }

    /* Reset hover */
    if (menu->hovered >= 0 && menu->items[menu->hovered].buf)
        set_item_buf(menu, menu->hovered, false);

    wlr_scene_node_set_enabled(&menu->tree->node, false);
    menu->visible = false;
    menu->hovered = -1;
}

/* ── Hit testing ──────────────────────────────────────────────────────────── */

bool menu_hit(const struct ywm_menu *menu, double lx, double ly) {
    int h = MENU_TITLE_H + menu->count * MENU_ITEM_H;
    return lx >= menu->x && lx < menu->x + MENU_WIDTH &&
           ly >= menu->y && ly < menu->y + h;
}

static int item_at(const struct ywm_menu *menu, double lx, double ly) {
    if (lx < menu->x || lx >= menu->x + MENU_WIDTH) return -1;
    double ry = ly - menu->y - MENU_TITLE_H;
    if (ry < 0) return -1;
    int idx = (int)(ry / MENU_ITEM_H);
    return (idx < menu->count) ? idx : -1;
}

/* ── Motion: hover highlight ─────────────────────────────────────────────── */

void menu_motion(struct ywm_menu *menu, double lx, double ly) {
    if (menu->flash_item >= 0) return;   /* don't change hover during flash */

    int idx = item_at(menu, lx, ly);
    if (idx == menu->hovered) return;

    if (menu->hovered >= 0)
        set_item_buf(menu, menu->hovered, false);

    menu->hovered = idx;

    if (idx >= 0)
        set_item_buf(menu, idx, true);   /* flash colour = hover highlight */
}

/* ── Activate: flash item then run command ───────────────────────────────── */

void menu_activate(struct ywm_menu *menu, double lx, double ly) {
    int idx = item_at(menu, lx, ly);
    if (idx < 0) return;   /* click on title bar — do nothing */

    /* Store command before any state changes */
    snprintf(menu->pending_cmd, MENU_CMD_MAX, "%s", menu->items[idx].command);

    /* Reset hover without re-rendering (flash will take over) */
    menu->hovered = -1;

    /* Start flash sequence: initial state = normal (i=0 in Xlib loop) */
    menu->flash_item  = idx;
    menu->flash_count = 0;
    set_item_buf(menu, idx, false);

    wl_event_source_timer_update(menu->flash_timer, FLASH_MS);
}
