#include "menu.h"
#include "util.h"

Window create_menu(Display *dpy, GC gc, int black_color, int white_color)
{
  XColor bg = create_color(dpy, "#AAAAAA");
  Window menu_win = XCreateSimpleWindow(dpy,
					DefaultRootWindow(dpy),
					0, 0, 175, 25 * 6, 0,
					black_color,
					bg.pixel);

  int len = sizeof(menu_items) / sizeof(menu_items[0]);
  printf("Creating root menu with %d items\n", len);
  printf("%lx\n", menu_win);
  for(int i = 0; i < len; i++)
  {
    Window menu_item = XCreateSimpleWindow(dpy, menu_win, 0, 25 * i, 175, 25, 0, black_color, bg.pixel);
      XSelectInput(dpy, menu_item, ButtonPressMask | ButtonReleaseMask | ExposureMask);
      menu_item_wins[i] = menu_item;
      printf("%lx\n", menu_item);
      
  }
  XMapSubwindows(dpy, menu_win);

  return menu_win;
}


