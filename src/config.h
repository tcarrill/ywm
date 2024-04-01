#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <X11/Xlib.h>
#include "util.h"
#include "ylist.h"
#include "ywm.h"

typedef struct {
  int window_resistance;
  int window_snap_buffer;
  int edge_resistance;
  int edge_snap_buffer;
  char background_color[8];
  char menu_title_color[8];
} YConfig;

YConfig* config;

void read_config();
#endif
