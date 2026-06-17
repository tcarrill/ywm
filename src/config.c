#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "config.h"

#define DEFAULT_BG       "#666794"
#define DEFAULT_MENU_BAR "#9b9bb9"

static void set_defaults(struct ywm_config *cfg) {
    cfg->bg_color[0] = 0x66 / 255.0f;
    cfg->bg_color[1] = 0x67 / 255.0f;
    cfg->bg_color[2] = 0x97 / 255.0f;
    cfg->bg_color[3] = 1.0f;

    cfg->menu_title_color[0] = 0x9b / 255.0f;
    cfg->menu_title_color[1] = 0x9b / 255.0f;
    cfg->menu_title_color[2] = 0xb9 / 255.0f;
    cfg->menu_title_color[3] = 1.0f;

    cfg->sloppy_focus        = true;
    cfg->window_snap_buffer  = 10;
    cfg->window_resistance   = 30;
    cfg->edge_snap_buffer    = 10;
    cfg->edge_resistance     = 50;
    cfg->tile_path[0]        = '\0';
    cfg->menu_alpha          = .75f;
    cfg->csd_app_count       = 0;
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
    fprintf(f, "menu_bar_color = " DEFAULT_MENU_BAR "\n");
    fprintf(f, "sloppy_focus = true\n");
    fprintf(f, "window_snap_buffer = 10\n");
    fprintf(f, "window_resistance = 30\n");
    fprintf(f, "edge_snap_buffer = 10\n");
    fprintf(f, "edge_resistance = 50\n");
    fprintf(f, "background_picture = ~/.config/ywm/backgrounds/default.png\n");
    fprintf(f, "menu_alpha = 75\n");
    fprintf(f, "csd_apps = firefox_firefox,chromium\n");
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
            else if (strcmp(key, "menu_title_color") == 0 ||
                     strcmp(key, "menu_bar_color") == 0)
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
            else if (strcmp(key, "csd_apps") == 0) {
                char *eq = strchr(p, '=');
                if (eq) {
                    char *v = eq + 1;
                    while (*v == ' ' || *v == '\t') v++;
                    /* trim trailing whitespace/newline */
                    size_t len = strlen(v);
                    while (len > 0 && (v[len-1] == '\n' || v[len-1] == '\r' ||
                                       v[len-1] == ' '  || v[len-1] == '\t'))
                        v[--len] = '\0';
                    /* split by comma */
                    char *tok = v;
                    while (*tok && cfg->csd_app_count < MAX_CSD_APPS) {
                        char *comma = strchr(tok, ',');
                        size_t tlen = comma ? (size_t)(comma - tok) : strlen(tok);
                        /* trim leading whitespace */
                        while (tlen > 0 && (*tok == ' ' || *tok == '\t'))
                            { tok++; tlen--; }
                        /* trim trailing whitespace */
                        while (tlen > 0 && (tok[tlen-1] == ' ' || tok[tlen-1] == '\t'))
                            tlen--;
                        if (tlen > 0) {
                            size_t copy = tlen < MAX_APP_ID - 1 ? tlen : MAX_APP_ID - 1;
                            memcpy(cfg->csd_apps[cfg->csd_app_count], tok, copy);
                            cfg->csd_apps[cfg->csd_app_count][copy] = '\0';
                            cfg->csd_app_count++;
                        }
                        if (!comma) break;
                        tok = comma + 1;
                    }
                }
            } else if (strcmp(key, "menu_alpha") == 0) {
                float pct = (float)atof(val);
                if (pct < 0.0f)   pct = 0.0f;
                if (pct > 100.0f) pct = 100.0f;
                cfg->menu_alpha = pct / 100.0f;
            } else if (strcmp(key, "background_picture") == 0) {
                /* Re-parse raw line to capture full path (may contain spaces) */
                char *eq = strchr(p, '=');
                if (eq) {
                    char *v = eq + 1;
                    while (*v == ' ' || *v == '\t') v++;
                    size_t len = strlen(v);
                    while (len > 0 && (v[len-1] == '\n' || v[len-1] == '\r' ||
                                       v[len-1] == ' '  || v[len-1] == '\t'))
                        v[--len] = '\0';
                    if (v[0] == '~' && home)
                        snprintf(cfg->tile_path, sizeof(cfg->tile_path),
                                 "%s%s", home, v + 1);
                    else
                        snprintf(cfg->tile_path, sizeof(cfg->tile_path), "%s", v);
                }
            }
        }
    }

    fclose(f);
}
