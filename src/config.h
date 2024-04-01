#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
  int window_resistance;
  int window_snap_buffer;
  int edge_resistance;
  int edge_snap_buffer;
  char background_color[8];
  char menu_title_color[8];
} YConfig;

extern YConfig* config;

void read_config();
#endif
