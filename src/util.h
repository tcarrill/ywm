#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include "client.h"
#include "ywm.h"

#define FRAME 0
#define CLIENT 1
#define CLOSE_BTN 2
#define SHADE_BTN 3

Client* find_client_by_type(Window win, int type);
Client* find_client(Window win);
void remove_client(Client* c);
void fork_exec(char *cmd);
void send_wm_delete(Window window);
XColor create_color(char* hex);
XColor create_color_shade(char* hex, float shade);
void print_client(Client* c);
int snap_window_right(int x);
int snap_window_left(int x);
int snap_window_top(int y);
int snap_window_bottom(int y);
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
