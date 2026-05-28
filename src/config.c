#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "config.h"

#define DEFAULT_BG "#666797"

static void set_defaults(struct ywm_config *cfg) {
    cfg->bg_color[0] = 0x66 / 255.0f;
    cfg->bg_color[1] = 0x67 / 255.0f;
    cfg->bg_color[2] = 0x97 / 255.0f;
    cfg->bg_color[3] = 1.0f;

    cfg->menu_title_color[0] = 0x9b / 255.0f;
    cfg->menu_title_color[1] = 0x9b / 255.0f;
    cfg->menu_title_color[2] = 0xbb / 255.0f;
    cfg->menu_title_color[3] = 1.0f;

    cfg->sloppy_focus        = false;
    cfg->window_snap_buffer  = 20;
    cfg->window_resistance   = 30;
    cfg->edge_snap_buffer    = 20;
    cfg->edge_resistance     = 50;
}

static void parse_hex_color(const char *val, float out[4]) {
    unsigned int r, g, b;
    if (sscanf(val, "#%02x%02x%02x", &r, &g, &b) == 3) {
        out[0] = r / 255.0f;
        out[1] = g / 255.0f;
        out[2] = b / 255.0f;
        out[3] = 1.0f;
    }
}

static void ensure_default_config(const char *path) {
    /* Create ~/.config/ywm/ if needed */
    char dir[512];
    snprintf(dir, sizeof(dir), "%s", path);
    char *slash = strrchr(dir, '/');
    if (slash) {
        *slash = '\0';
        mkdir(dir, 0755);
    }

    FILE *f = fopen(path, "wx"); /* 'x' = fail if exists */
    if (!f) return;
    fprintf(f, "background_color = " DEFAULT_BG "\n");
    fprintf(f, "sloppy_focus = false\n");
    fprintf(f, "window_snap_buffer = 20\n");
    fprintf(f, "window_resistance = 30\n");
    fprintf(f, "edge_snap_buffer = 20\n");
    fprintf(f, "edge_resistance = 50\n");
    fclose(f);
}

void config_load(struct ywm_config *cfg) {
    set_defaults(cfg);

    const char *home = getenv("HOME");
    if (!home) return;

    char path[512];
    snprintf(path, sizeof(path), "%s/.config/ywm/ywm.ini", home);

    ensure_default_config(path);

    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == ';' || *p == '\n' || *p == '\0') continue;

        char key[64], val[64];
        if (sscanf(p, "%63[^= \t] = %63s", key, val) == 2) {
            if (strcmp(key, "background_color") == 0)
                parse_hex_color(val, cfg->bg_color);
            else if (strcmp(key, "menu_title_color") == 0)
                parse_hex_color(val, cfg->menu_title_color);
            else if (strcmp(key, "sloppy_focus") == 0)
                cfg->sloppy_focus = (strcmp(val, "true") == 0);
            else if (strcmp(key, "window_snap_buffer") == 0)
                cfg->window_snap_buffer = atoi(val);
            else if (strcmp(key, "window_resistance") == 0)
                cfg->window_resistance = atoi(val);
            else if (strcmp(key, "edge_snap_buffer") == 0)
                cfg->edge_snap_buffer = atoi(val);
            else if (strcmp(key, "edge_resistance") == 0)
                cfg->edge_resistance = atoi(val);
        }
    }

    fclose(f);
}
