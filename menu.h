#ifndef MENU_H
#define MENU_H

#include <stdio.h>
#include <X11/Xlib.h>

enum menu_item_type 
{
  Command,
  Menu
};

struct menu_item {
  enum menu_item_type type;
  char *label;
  char *command;
};

static struct menu_item menu_items[] =
{
      { .label = "Xterm", .command = "xterm", .type = Command },
      { .label = "Xcalc", .command = "xcalc", .type = Command },
      { .label = "Xclock", .command = "xclock", .type = Command },
      { .label = "Xeyes", .command = "xeyes", .type = Command },
      { .label = "Xlogo", .command = "xlogo", .type = Command },
      { .label = "Exit ywm", .command = "exit", .type = Command }
};

Window menu_item_wins[6];

Window create_menu(Display *dpy, GC gc, int black_color, int white_color);

#endif
