#include "menu.h"

#define TITLE_BAR_HEIGHT 0

#define MENU_TITLE_COLOR "#999aba"
#define MENU_TITLE_LIGHT_STRIP "#bfbfcc"
#define MENU_TITLE_DARK_STRIP "#5d5d68"
#define MENU_BG_COLOR "#AAAAAA"
#define MENU_BORDER_DARK "#555555"
#define MENU_BORDER_LIGHT "#EFEFEF"
#define MENU_BORDER_LIGHT2 "#ACACAC"
#define FLASH_COLOR "#ccc8d1"

GC bg_gc;
GC flash_gc;

Window create_menu()
{
  XColor bg = create_color(MENU_BG_COLOR);
  XGCValues gcv;
  gcv.function = GXcopy;

  gcv.foreground = create_color(MENU_BORDER_DARK).pixel;
  menu_border_dark_gc = XCreateGC(dpy, root, GCFunction|GCForeground, &gcv);
  gcv.foreground = create_color(MENU_BORDER_LIGHT).pixel;
  menu_border_light_gc = XCreateGC(dpy, root, GCFunction|GCForeground, &gcv);
  gcv.foreground = create_color(MENU_BORDER_LIGHT2).pixel;
  menu_border_light2_gc = XCreateGC(dpy, root, GCFunction|GCForeground, &gcv);
  gcv.foreground = create_color(FLASH_COLOR).pixel;
  flash_gc = XCreateGC(dpy, root, GCFunction|GCForeground, &gcv);
  gcv.foreground = bg.pixel;
  bg_gc = XCreateGC(dpy, root, GCFunction|GCForeground, &gcv);
  
  gcv.foreground = create_color(MENU_TITLE_COLOR).pixel;
  menu_title_gc = XCreateGC(dpy, root, GCFunction|GCForeground, &gcv);
  gcv.foreground = create_color(MENU_TITLE_LIGHT_STRIP).pixel;
  menu_light_strip_gc = XCreateGC(dpy, root, GCFunction|GCForeground, &gcv);
  gcv.foreground = create_color(MENU_TITLE_DARK_STRIP).pixel;
  menu_dark_strip_gc = XCreateGC(dpy, root, GCFunction|GCForeground, &gcv);
  
  Window menu_win = XCreateSimpleWindow(dpy,
					DefaultRootWindow(dpy),
					0, 0, MENU_MAX_WIDTH, MENU_ITEM_HEIGHT * MENU_SIZE + TITLE_BAR_HEIGHT, 0,
					BlackPixel(dpy, 0),
					bg.pixel);
  XSelectInput(dpy, menu_win, ButtonPressMask | ButtonReleaseMask | ExposureMask);

  for(int i = 0; i < MENU_SIZE; i++)
  {
      Window menu_item = XCreateSimpleWindow(dpy,
        menu_win,
        0, MENU_ITEM_HEIGHT * i + TITLE_BAR_HEIGHT, MENU_MAX_WIDTH, MENU_ITEM_HEIGHT, 0,
        BlackPixel(dpy, 0),
        bg.pixel);
		
      XSelectInput(dpy, menu_item, ButtonPressMask | ButtonReleaseMask | ExposureMask);

	  menu_item_wins[i] = menu_item;
  }
  XMapSubwindows(dpy, menu_win);

  return menu_win;
}

static void draw_menu_item(int menu_item, int flash) {
  int width = MENU_MAX_WIDTH - 1;
  int height = MENU_ITEM_HEIGHT - 1;
  GC defaultGC = DefaultGC(dpy, DefaultScreen(dpy));
  Window menu_btn = menu_item_wins[menu_item];
  if (menu_item > 0) {
	  XDrawLine(dpy, menu_btn, menu_border_light_gc, 0, 0, width, 0);
	  XDrawLine(dpy, menu_btn, menu_border_light2_gc, 1, 1, width, 1);
	  XDrawLine(dpy, menu_btn, menu_border_light_gc, 0, 1, 0, height);
	  XDrawLine(dpy, menu_btn, menu_border_light2_gc, 1, 1, 1, height);
	  XDrawLine(dpy, menu_btn, menu_border_dark_gc, 1, height - 1, width, height - 1);
	  XDrawLine(dpy, menu_btn, menu_border_dark_gc, width - 1, 1, width - 1, height - 1);
	  XDrawLine(dpy, menu_btn, defaultGC, width, 0, width, height);
	  XDrawLine(dpy, menu_btn, defaultGC, 0, height, width, height);
	  if (flash) {
	    XFillRectangle(dpy, menu_btn, flash_gc, 0, 0, width - 1, height - 2);
	  } else {
	    XFillRectangle(dpy, menu_btn, bg_gc, 2, 2, width - 3, height - 3);
	  }
	  if (strlen(menu_items[menu_item].label) > 0) {
		  XDrawString(dpy, menu_btn, defaultGC, 7, 15, menu_items[menu_item].label, strlen(menu_items[menu_item].label));
	  }
  } else {
	  XFillRectangle(dpy, menu_btn, menu_title_gc, 0, 0, width, height);
	  XDrawLine(dpy, menu_btn, menu_border_light_gc, 0, 0, width, 0);
	  XDrawLine(dpy, menu_btn, menu_border_light2_gc, 1, 1, width, 1);
	  XDrawLine(dpy, menu_btn, menu_border_light_gc, 0, 1, 0, height);
	  XDrawLine(dpy, menu_btn, menu_border_light2_gc, 1, 1, 1, height);
	  XDrawLine(dpy, menu_btn, menu_border_dark_gc, 1, height - 1, width, height - 1);
	  XDrawLine(dpy, menu_btn, menu_border_dark_gc, width - 1, 1, width - 1, height - 1);
	  XDrawLine(dpy, menu_btn, defaultGC, width, 0, width, height);
	  XDrawLine(dpy, menu_btn, defaultGC, 0, height, width, height);
      for (int i = 0; i < 12; i++) {
		int y = 5 + i;
    	if (i % 2 == 0) {
   			XDrawLine(dpy, menu_btn, menu_light_strip_gc, 5, y, MENU_MAX_WIDTH - 8, y);
   		} else {
   			XDrawLine(dpy, menu_btn, menu_dark_strip_gc, 6, y, MENU_MAX_WIDTH - 7, y);
		}
     }	
  }
}

void draw_menu() {	
  for (int i = 0; i < MENU_SIZE; i++) {
    draw_menu_item(i, 0);
  }
}

void flash_menu(int menu_item) {
    for (int i = 0; i < MENU_SIZE; i++) {
      if (i % 2 != 0) {
        draw_menu_item(menu_item, 1);
      } else {
        draw_menu_item(menu_item, 0);
      }

      XSync(dpy, True);
      usleep(22500);
    }
}

void free_menu() {
  XFreeGC(dpy, menu_border_light_gc);
  XFreeGC(dpy, menu_border_light2_gc);
  XFreeGC(dpy, menu_border_dark_gc);
  XFreeGC(dpy, menu_title_gc);
  XFreeGC(dpy, bg_gc);
  XFreeGC(dpy, flash_gc);
}
