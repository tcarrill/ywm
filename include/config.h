#ifndef YWM_CONFIG_H
#define YWM_CONFIG_H

#include <stdbool.h>

struct ywm_config {
    float bg_color[4];          /* RGBA, [0,1] — default #666797 */
    float menu_title_color[4];  /* RGBA, [0,1] — default #9b9bbb */
    bool  sloppy_focus;         /* focus-follows-mouse, no raise */
    int   window_snap_buffer;   /* px proximity to trigger window snap */
    int   window_resistance;    /* px range over which snap holds */
    int   edge_snap_buffer;     /* px proximity to trigger screen-edge snap */
    int   edge_resistance;      /* px range over which edge snap holds */
    char  tile_path[512];       /* path to PNG tile; empty = use solid bg_color */
    float menu_alpha;           /* menu item opacity 0.0–1.0; default 1.0 */

#define MAX_CSD_APPS 32
#define MAX_APP_ID   128
    char csd_apps[MAX_CSD_APPS][MAX_APP_ID];  /* app_ids treated as CSD */
    int  csd_app_count;
};

void config_load(struct ywm_config *cfg);

#endif /* YWM_CONFIG_H */
