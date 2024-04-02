#ifndef YWM_H
#define YWM_H
#include "ylist.h"
#include "client.h"
#include "menu.h"
#include "config.h"

#define YWM_DIR ".ywm"

#define FRAME_TITLEBAR_HEIGHT 20
#define FRAME_BORDER_WIDTH 4
#define X_BORDER_WIDTH 1
#define TOTAL_FRAME_WIDTH ((2 * FRAME_BORDER_WIDTH) + (2 * X_BORDER_WIDTH))
#define TOTAL_FRAME_HEIGHT (FRAME_TITLEBAR_HEIGHT + FRAME_BORDER_WIDTH + (2 * X_BORDER_WIDTH))

#define MIN_WIDTH 50
#define MIN_HEIGHT 40

#define FRAME_CORNER_OFFSET 20

#define FOCUSED_FRAME_COLOR "#aaaaaa"
#define FOCUSED_LIGHT_GREY "#cacaca"
#define FOCUSED_DARK_GREY "#6a6a6a"

#define UNFOCUSED_FRAME_COLOR "#cccccc"
#define UNFOCUSED_LIGHT_GREY "#eaeaea"
#define UNFOCUSED_DARK_GREY "#767676"

#define SPACE 3

#define ButtonMask ButtonPressMask | ButtonReleaseMask

extern char ywm_path[PATH_MAX];
extern YConfig* config;

extern GC focused_light_grey_gc;
extern GC focused_dark_grey_gc;

extern GC unfocused_light_grey_gc;
extern GC unfocused_dark_grey_gc;

extern GC focused_frame_gc;
extern GC unfocused_frame_gc;

extern Display *dpy;
extern Window root;
extern Window root_menu;
extern Client *focused_client;

extern int screen_w, screen_h;

extern XftFont *title_xft_font;
extern XftColor xft_color;

enum AtomsWM {
  AtomWMDeleteWindow,
  AtomWMProtocols,
  NumberOfAtoms
};

extern Atom atom_wm[NumberOfAtoms];

extern Point cursor_start_point;
extern Point cursor_start_win_point;
extern Rect start_window_geom;
extern Rect current_window_geom;

extern Cursor pointerCursor;

extern YList clients;
extern YList focus_stack;

extern Bool shape;
extern int shape_event;

void set_shape(Client *);
void frame(Window root, Window win);
void unframe(Window win);
void redraw(Client *client);
void handle_shading(Client *);
void shade(Client *);
void unshade(Client *);
#endif
