#ifndef YWM_H
#define YWM_H
#include <stdlib.h>
#include <errno.h>
// #include <X11/Xft/Xft.h>
#include "ylist.h"
#include "client.h"
#include "menu.h"

#define DEF_FONT "-b&h-lucida-bold-r-*-*-11-*-*-*-*-*-*-*"
// #define DEF_FONT "-bitstream-bitstream vera sans-medium-r-*-*-*-100-*-*-*-*-*-*"
#define FRAME_TITLEBAR_HEIGHT 20
#define FRAME_BORDER_WIDTH 4

#define FOCUSED_FRAME_COLOR "#aaaaaa"
#define FOCUSED_LIGHT_GREY "#cacaca"
#define FOCUSED_DARK_GREY "#6a6a6a"

#define UNFOCUSED_FRAME_COLOR "#cccccc"
#define UNFOCUSED_LIGHT_GREY "#eaeaea"
#define UNFOCUSED_DARK_GREY "#767676"

#define RED "#fc5b57"
#define LIGHT_RED "#ed8887"
#define DARK_RED "#c45554"
#define BLUE "#0066cc"
#define SPACE 3

#define ButtonMask ButtonPressMask | ButtonReleaseMask

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

int screen_w, screen_h;

// XftFont *xftfont = NULL;
// XftColor xft_detail;

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

void frame(Window root, Window win);
void unframe(Window win);
void redraw(Client *client);
#endif
