#include <X11/Xlib.h>
#include "util.h"

XColor create_color(Display *dpy, char *hex)
{
  XColor color;
  Colormap colormap = DefaultColormap(dpy, 0);
  XParseColor(dpy, colormap, hex, &color);
  XAllocColor(dpy, colormap, &color);
  return color;
}

