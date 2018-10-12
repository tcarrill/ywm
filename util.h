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

Client* find_client_by_type(Window win, int type);
Client* find_client(Window win);
void remove_client(Display* dpy, Client* c);
void fork_exec(char *cmd);
void send_wm_delete(Window window);
XColor create_color(char* hex);
void print_client(Client* c);

#endif
