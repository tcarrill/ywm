#include "menu.h"
#include "util.h"

Window create_menu(Display *dpy, GC gc, int black_color, int white_color)
{
  Window menu_win = XCreateSimpleWindow(dpy,
					DefaultRootWindow(dpy),
					0, 0, 200, 25 * 6 + 1, 0,
					black_color,
					white_color);

  int len = sizeof(menu_items) / sizeof(menu_items[0]);
  printf("Creating root menu with %d items\n", len);
  
  XColor bg = create_color(dpy, "#A0A0A0");
  for(int i = 0; i < len; i++)
  {
    Window menu_item = XCreateSimpleWindow(dpy, menu_win, 0, 25 * i, 198, 24, 1, black_color, bg.pixel);
      XSelectInput(dpy, menu_item, ButtonPressMask | ButtonReleaseMask | ExposureMask);
      menu_item_wins[i] = menu_item;
      printf("%lx - %s: %s\n", menu_item, menu_items[i].label, menu_items[i].command);
  }
  XMapSubwindows(dpy, menu_win);

  return menu_win;
}

