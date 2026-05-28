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
};

void config_load(struct ywm_config *cfg);

#endif /* YWM_CONFIG_H */
