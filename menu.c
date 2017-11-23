#include "menu.h"

Window create_menu()
{
  XColor dark = create_color(MENU_BORDER_DARK);
  XColor light = create_color(MENU_BORDER_LIGHT);
  XColor light2 = create_color(MENU_BORDER_LIGHT2);
  XGCValues gcv;
  gcv.function = GXcopy;

  gcv.foreground = dark.pixel;
  menu_border_dark_gc = XCreateGC(dpy, root, GCFunction|GCForeground, &gcv);
  gcv.foreground = light.pixel;
  menu_border_light_gc = XCreateGC(dpy, root, GCFunction|GCForeground, &gcv);
  gcv.foreground = light2.pixel;
  menu_border_light2_gc = XCreateGC(dpy, root, GCFunction|GCForeground, &gcv);

  XColor bg = create_color(MENU_BG_COLOR);
  Window menu_win = XCreateSimpleWindow(dpy,
					DefaultRootWindow(dpy),
					0, 0, MENU_MAX_WIDTH, MENU_ITEM_HEIGHT * MENU_SIZE, 0,
					BlackPixel(dpy, 0),
					bg.pixel);

  for(int i = 0; i < MENU_SIZE; i++)
  {
      Window menu_item = XCreateSimpleWindow(dpy, menu_win,
        0, MENU_ITEM_HEIGHT * i, MENU_MAX_WIDTH, MENU_ITEM_HEIGHT, 0,
        BlackPixel(dpy, 0), bg.pixel);
      XSelectInput(dpy, menu_item, ButtonPressMask | ButtonReleaseMask | ExposureMask);
      menu_item_wins[i] = menu_item;
  }
  XMapSubwindows(dpy, menu_win);

  return menu_win;
}

void draw_menu() {
  int width = MENU_MAX_WIDTH - 1;
  int height = MENU_ITEM_HEIGHT - 1;
  GC defaultGC = DefaultGC(dpy, DefaultScreen(dpy));

  for (int i = 0; i < MENU_SIZE; i++) {
    Window menu_btn = menu_item_wins[i];
    XWindowAttributes menu_item_attr;
    XGetWindowAttributes(dpy, menu_btn, &menu_item_attr);

    XDrawLine(dpy, menu_btn, menu_border_light_gc, 0, 0, width, 0);
    XDrawLine(dpy, menu_btn, menu_border_light2_gc, 1, 1, width, 1);
    XDrawLine(dpy, menu_btn, menu_border_light_gc, 0, 1, 0, height);
    XDrawLine(dpy, menu_btn, menu_border_light2_gc, 1, 1, 1, height);
    XDrawLine(dpy, menu_btn, menu_border_dark_gc, 1, height - 1, width, height - 1);
    XDrawLine(dpy, menu_btn, menu_border_dark_gc, width - 1, 1, width - 1, height - 1);
    XDrawLine(dpy, menu_btn, defaultGC, width, 0, width, height);
    XDrawLine(dpy, menu_btn, defaultGC,0, height, width, height);
    XDrawString(dpy, menu_btn, defaultGC, 7, 16, menu_items[i].label, strlen(menu_items[i].label));
  }
}
