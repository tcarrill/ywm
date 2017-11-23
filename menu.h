#ifndef MENU_H
#define MENU_H

#include <stdio.h>
#include <X11/Xlib.h>
#include "util.h"
#include "ywm.h"

#define MENU_MAX_WIDTH  175
#define MENU_ITEM_HEIGHT 25
#define MENU_BG_COLOR "#AAAAAA"
#define MENU_BORDER_DARK "#555555"
#define MENU_BORDER_LIGHT "#D4D4D4"
#define MENU_BORDER_LIGHT2 "#ACACAC"
#define MENU_SIZE  6

GC menu_border_dark_gc, menu_border_light_gc, menu_border_light2_gc;

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

// todo: menu_items should be a linked list
static struct menu_item menu_items[] =
{
      { .label = "Xterm", .command = "xterm", .type = Command },
      { .label = "Xcalc", .command = "xcalc", .type = Command },
      { .label = "Xclock", .command = "xclock", .type = Command },
      { .label = "Xeyes", .command = "xeyes", .type = Command },
      { .label = "Xlogo", .command = "xlogo", .type = Command },
      { .label = "Exit ywm", .command = "exit", .type = Command }
};

// todo: linked list
Window menu_item_wins[MENU_SIZE];

Window create_menu();
void draw_menu();

#endif
