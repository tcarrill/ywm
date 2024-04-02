#ifndef UTIL_H
#define UTIL_H

#include <X11/Xlib.h>
#include <X11/Xcms.h>
#include "client.h"

#define FRAME 0
#define CLIENT 1
#define CLOSE_BTN 2
#define SHADE_BTN 3

extern YConfig* config;

Client* find_client_by_type(Window win, int type);
Client* find_client(Window win);
void remove_client(Client* c);
void fork_exec(char *cmd);
void send_wm_delete(Window window);
XcmsColor create_hvc_color(char* hex);
XColor create_color(char* hex);
void print_client(Client* c);
int snap_window_screen_right(int x);
int snap_window_screen_left(int x);
int snap_window_screen_top(int y);
int snap_window_screen_bottom(int y);
int snap_window_right(Rect a, Rect b);
int snap_window_left(Rect a, Rect b);
int snap_window_top(Rect a, Rect b);
int snap_window_bottom(Rect a, Rect b);
int is_title_bar(Point point);
int is_left_frame(int x);
int is_right_frame(int x);
int is_bottom_frame(int y);
int is_lower_left_corner(Point point);
int is_lower_right_corner(Point point);
int is_resize_frame(Point point);
int handle_xerror(Display *dpy, XErrorEvent *e);
void print_event(XEvent ev);
int mkdir_p(const char *path);
#endif
