#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <X11/Xlib.h>

XColor create_color(Display *dpy, char* hex);

#endif
