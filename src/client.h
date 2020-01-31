#ifndef CLIENT_H
#define CLIENT_H

#include <X11/Xft/Xft.h>

typedef struct {
  int x, y;
} Point;

typedef struct {
  int x, y;
  int width, height;
} Rect;

typedef struct {
  char *title;
  Window close_button;
  Window client;
  Window frame;
  XftDraw *xft_draw;
  int shaded;
} Client;

#endif
