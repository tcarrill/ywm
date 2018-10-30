#ifndef YWM_H
#define YWM_H
#include <stdlib.h>
#include "ylist.h"
#include "client.h"
#include "menu.h"

#define DEF_FONT "-b&h-lucida-bold-r-*-*-11-*-*-*-*-*-*-*"
#define FRAME_TITLEBAR_HEIGHT 20
#define FRAME_BORDER_WIDTH 4

#define FOCUSED_FRAME_COLOR "#aaaaaa"
#define FOCUSED_LIGHT_GREY "#cacaca"
#define FOCUSED_DARK_GREY "#6a6a6a"

#define UNFOCUSED_FRAME_COLOR "#bbbbbb"
#define UNFOCUSED_LIGHT_GREY "#eaeaea"
#define UNFOCUSED_DARK_GREY "#767676"

#define RED "#fc5b57"

GC focused_light_grey_gc;
GC focused_dark_grey_gc;

GC unfocused_light_grey_gc;
GC unfocused_dark_grey_gc;

GC focused_frame_gc;
GC unfocused_frame_gc;

XFontStruct *title_font;
GC text_gc;

Display *dpy;
Window root;
Window root_menu;
Client *focused_client;

enum AtomsWM {
  AtomWMDeleteWindow,
  AtomWMProtocols,
  NumberOfAtoms
};

Atom atom_wm[NumberOfAtoms];

Point cursor_start_point;
Rect window_start;

YList clients;
YList focus_stack;

void frame(Display *dpy, Window root, Window win);
void unframe(Display *dpy, Window win);
void redraw(Display *dpy, Client *client);
#endif
