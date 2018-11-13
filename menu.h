#ifndef MENU_H
#define MENU_H

#include <stdio.h>
#include <X11/Xlib.h>
#include "util.h"
#include "ylist.h"
#include "ywm.h"

#define MENU_MAX_WIDTH  175
#define MENU_ITEM_HEIGHT 22

GC menu_border_dark_gc, menu_border_light_gc, menu_border_light2_gc, menu_title_gc, menu_light_strip_gc, menu_dark_strip_gc;

enum menu_item_type
{
  Command,
  Menu
};

typedef struct MenuItem {
  enum menu_item_type type;
  char *label;
  char *command;
  Window window;
} MenuItem;

YList menu_items;

Window create_menu();
void draw_menu();
void free_menu();
void flash_menu();
#endif
