#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <X11/Xlib.h>

void fork_exec(char *cmd);
XColor create_color(Display *dpy, char* hex);

#endif
