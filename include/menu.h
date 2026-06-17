#ifndef YWM_MENU_H
#define YWM_MENU_H

#include <stdbool.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_scene.h>

/* Dimensions matching the original Xlib ywm menu exactly */
#define MENU_MAX_ITEMS  32
#define MENU_LABEL_MAX  64
#define MENU_CMD_MAX   256
#define MENU_WIDTH     175   /* MENU_MAX_WIDTH  in Xlib */
#define MENU_ITEM_H     22   /* MENU_ITEM_HEIGHT in Xlib */
#define MENU_TITLE_H    22   /* TITLE_BAR_HEIGHT in Xlib menu.c */
#define FLASH_MS        42   /* ~41.67 ms per flash tick (1/24 s) */

struct ywm_server;

struct ywm_menu_item {
    char label[MENU_LABEL_MAX];
    char command[MENU_CMD_MAX];
    struct wlr_scene_buffer *buf;   /* single Cairo buffer: bevel + fill + text */
};

struct ywm_menu {
    bool visible;
    int  x, y;
    int  count;
    int  hovered;        /* index of hovered item, -1 = none */

    struct ywm_menu_item    items[MENU_MAX_ITEMS];
    struct wlr_scene_tree  *tree;
    struct wlr_scene_buffer *title_buf;

    float title_color[4];   /* copied from cfg at init; R G B A in [0,1] */
    float alpha;            /* item fill opacity 0.0–1.0; title bar always opaque */

    /* Flash animation state */
    struct wl_event_source *flash_timer;
    int   flash_item;        /* index of item being animated, -1 = idle */
    int   flash_count;       /* how many timer ticks have fired */
    char  pending_cmd[MENU_CMD_MAX];
};

void menu_init           (struct ywm_menu *menu, struct ywm_server *server);
void menu_load           (struct ywm_menu *menu);
void menu_show           (struct ywm_menu *menu, int x, int y);
void menu_hide           (struct ywm_menu *menu);
void menu_motion         (struct ywm_menu *menu, double lx, double ly);
void menu_activate       (struct ywm_menu *menu, double lx, double ly);
bool menu_hit            (const struct ywm_menu *menu, double lx, double ly);
void menu_config_changed (struct ywm_menu *menu);

#endif /* YWM_MENU_H */
