#ifndef YWM_H
#define YWM_H
#include <stdlib.h>
#include "ylist.h"

#define STRIP_COLOR1 0xcacaca
#define STRIP_COLOR2 0x6a6a6a

Display *dpy;
Window root;
Window close_button;

enum AtomsWM {
  AtomWMDeleteWindow,
  AtomWMProtocols,
  NumberOfAtoms
};

Atom atom_wm[NumberOfAtoms];

typedef struct {
	int x, y;
} Point;

typedef struct {
	int x, y;
	int width, height;
} Rect;

typedef struct {
	Window close_button;
	Window client;
	Window frame;
} Client;

Point cursor_start_point;
Rect window_start;

// TODO: Add client to frame hashmap instead of list
YList* clients;

void frame(Display *dpy, Window root, Window win);
#endif
