#ifndef YWM_H
#define YWM_H
#include <stdlib.h>
#include "ylist.h"
#include "client.h"
#include "menu.h"

#define DEF_FONT "-b&h-lucida-bold-r-*-*-11-*-*-*-*-*-*-*"

XFontStruct *title_font;
GC text_gc;

Display *dpy;
Window root;
Window root_menu;

enum AtomsWM {
  AtomWMDeleteWindow,
  AtomWMProtocols,
  NumberOfAtoms
};

Atom atom_wm[NumberOfAtoms];

Point cursor_start_point;
Rect window_start;

// TODO: Add client to frame hashmap instead of list
YList clients;

void frame(Display *dpy, Window root, Window win);
void unframe(Display *dpy, Window win);
void redraw(Display *dpy, Client *client);
#endif
