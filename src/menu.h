#ifndef MENU_H
#define MENU_H

#include <stdio.h>
#include <X11/Xlib.h>
#include "util.h"
#include "ylist.h"
#include "ywm.h"
#include "config.h"

#define MENU_MAX_WIDTH  175
#define MENU_ITEM_HEIGHT 22

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
  XftDraw *xft_draw;
} MenuItem;

extern XcmsColor menu_color;
extern YConfig* config;
extern XftFont *menu_xft_font;
extern YList menu_items;

Window create_menu();
void draw_menu();
void free_menu();
void flash_menu(MenuItem* menu_item);
void *poll_menu_file(void *ptr);
#endif
