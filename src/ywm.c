#include <string.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <signal.h>
#include "menu.h"
#include "util.h"
#include "ywm.h"
#include "event.h"
	
GC light_red_gc;
GC dark_red_gc; 
GC black_gc;

static void setup_wm_hints() 
{
  atom_wm[AtomWMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
  atom_wm[AtomWMDeleteWindow] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
}

static void setup_display() 
{
  
  if (!(dpy = XOpenDisplay(0x0))) {
    fprintf(stderr, "Unable to open display\n");
    exit(EXIT_FAILURE);
  }

  root = DefaultRootWindow(dpy);
  Screen* screen = XScreenOfDisplay(dpy, XDefaultScreen(dpy));
  screen_w = XWidthOfScreen(screen);
  screen_h = XHeightOfScreen(screen);

  XColor backgroundColor = create_color("#666797");
  XSetWindowBackground(dpy, root, backgroundColor.pixel);
  
  xft_color.color.red = 0; //65535
  xft_color.color.green = 0;
  xft_color.color.blue = 0;
  xft_color.color.alpha = 65535;
  xft_color.pixel = 0;

  title_xft_font = XftFontOpenName(dpy, DefaultScreen(dpy), "Arial-10:bold");
  if (title_xft_font == NULL)
  {
  	printf("font '%s' not found", "Arial-10:bold");
   	exit(EXIT_FAILURE);
  }
  
  XGCValues gcv;
  gcv.function = GXcopy;
  gcv.foreground = 0x000000;
  
  gcv.foreground = create_color(FOCUSED_LIGHT_GREY).pixel;
  focused_light_grey_gc = XCreateGC(dpy, root, GCFunction | GCForeground, &gcv);

  gcv.foreground = create_color(FOCUSED_DARK_GREY).pixel;
  focused_dark_grey_gc = XCreateGC(dpy, root, GCFunction | GCForeground, &gcv);

  gcv.foreground = create_color(UNFOCUSED_LIGHT_GREY).pixel;
  unfocused_light_grey_gc = XCreateGC(dpy, root, GCFunction | GCForeground, &gcv);

  gcv.foreground = create_color(UNFOCUSED_DARK_GREY).pixel;
  unfocused_dark_grey_gc = XCreateGC(dpy, root, GCFunction | GCForeground, &gcv);

  gcv.foreground = create_color(FOCUSED_FRAME_COLOR).pixel;
  focused_frame_gc = XCreateGC(dpy, root, GCFunction | GCForeground, &gcv);

  gcv.foreground = create_color(LIGHT_RED).pixel;
  light_red_gc = XCreateGC(dpy, root, GCFunction | GCForeground, &gcv);

  gcv.foreground = create_color(DARK_RED).pixel;
  dark_red_gc = XCreateGC(dpy, root, GCFunction | GCForeground, &gcv);
  
  gcv.foreground = 0x000000;
  black_gc = XCreateGC(dpy, root, GCFunction | GCForeground, &gcv);

  XClearWindow(dpy, root);

  pointerCursor = XCreateFontCursor(dpy, XC_left_ptr);
  resize_v = XCreateFontCursor(dpy, XC_sb_v_double_arrow);
  resize_h = XCreateFontCursor(dpy, XC_sb_h_double_arrow);

  XDefineCursor(dpy, root, pointerCursor);
}

void draw_close_button(Client *client, Rect initial_window)
{	
  if (client == focused_client) {
    XSetWindowBackground(dpy, client->close_button, create_color(RED).pixel);
    XClearWindow(dpy, client->close_button);
    XDrawLine(dpy, client->close_button, focused_dark_grey_gc, 0, 0, 12, 0);
    XDrawLine(dpy, client->close_button, focused_dark_grey_gc, 0, 0, 0, 12);
    XDrawLine(dpy, client->close_button, focused_light_grey_gc, 0, 12, 12, 12);
    XDrawLine(dpy, client->close_button, focused_light_grey_gc, 12, 12, 12, 0);
		
    XDrawLine(dpy, client->close_button, light_red_gc, 2, 2, 10, 2);
    XDrawLine(dpy, client->close_button, light_red_gc, 2, 2, 2, 10);
    XDrawLine(dpy, client->close_button, dark_red_gc, 2, 10, 10, 10);
    XDrawLine(dpy, client->close_button, dark_red_gc, 10, 2, 10, 10);
		
    XDrawLine(dpy, client->close_button, black_gc, 1, 1, 11, 1);
    XDrawLine(dpy, client->close_button, black_gc, 1, 1, 1, 11);
    XDrawLine(dpy, client->close_button, black_gc, 1, 11, 11, 11);
    XDrawLine(dpy, client->close_button, black_gc, 11, 11, 11, 1);
  } else {
    XSetWindowBackground(dpy, client->close_button, create_color(UNFOCUSED_FRAME_COLOR).pixel);
    XClearWindow(dpy, client->close_button);
  }
}

void draw_window_titlebar(Client *client, Rect initial_window) 
{ 	
  int yoffset = 4;
  int left_xstart_light = 19;
  int left_xstart_dark = left_xstart_light + 1;
  GC light_gc, dark_gc;
	
  if (client == focused_client) {
    light_gc = focused_light_grey_gc;
    dark_gc = focused_dark_grey_gc;
  } else {
    light_gc = unfocused_light_grey_gc;
    dark_gc = unfocused_dark_grey_gc;
  }
	
  if (client->title != NULL) {
    int title_len = strlen(client->title);
	XGlyphInfo *extents = malloc(sizeof(XGlyphInfo));
	XftTextExtents8(dpy, title_xft_font, (unsigned char *)client->title, title_len, extents);
	int title_width = extents->width;
    int titlex = (initial_window.width / 2) - (title_width / 2);
		
    if (client == focused_client) {
		xft_color.color.alpha = 65535;
      int left_xend_light = titlex - 10;
      int right_xstart_light = titlex + title_width + 7;
      int right_xend_light = initial_window.width - 7;

      int left_xend_dark = left_xend_light + 1;
      int right_xstart_dark = right_xstart_light + 1;
      int right_xend_dark = right_xend_light + 1;

      for (int i = 0; i < 12; i++) {
        int y = yoffset + i;
        if (i % 2 == 0) {
          XDrawLine(dpy, client->frame, light_gc, left_xstart_light, y, left_xend_light, y);
          XDrawLine(dpy, client->frame, light_gc, right_xstart_light, y, right_xend_light, y);
        } else {
          XDrawLine(dpy, client->frame, dark_gc, left_xstart_dark, y, left_xend_dark, y);
          XDrawLine(dpy, client->frame, dark_gc, right_xstart_dark, y, right_xend_dark, y);
        }
      }
    } else {
		xft_color.color.alpha = 32768;
    }
	
    XftDrawString8(client->xft_draw, &xft_color, title_xft_font, titlex, 14, (unsigned char *)client->title, title_len);
  } else {
    int xend_light = initial_window.width - 5;
    int xend_dark = xend_light + 1;
		
    for (int i = 0; i < 12; i++) {
      int y = yoffset + i;
      if (i % 2 == 0) {
        XDrawLine(dpy, client->frame, light_gc, left_xstart_light, y, xend_light, y);
      } else {
        XDrawLine(dpy, client->frame, dark_gc, left_xstart_dark, y, xend_dark, y);
      }
    }	
  }
}

void redraw(Client *client) 
{ 
  Window returned_root;
  int x, y;
  unsigned width, height, border_width, depth;
  XGetGeometry(
               dpy,
               client->frame,
               &returned_root,
               &x, &y,
               &width, &height,
               &border_width,
               &depth);
	
  GC dark_gc, light_gc;
  if (client == focused_client) {
    XSetWindowBackground(dpy, client->frame, create_color(FOCUSED_FRAME_COLOR).pixel);
    light_gc = focused_light_grey_gc;
    dark_gc = focused_dark_grey_gc;
  } else {
    XSetWindowBackground(dpy, client->frame, create_color(UNFOCUSED_FRAME_COLOR).pixel);
    light_gc = unfocused_light_grey_gc;
    dark_gc = unfocused_dark_grey_gc;
  }
  XClearWindow(dpy, client->frame);

  // light border top
  XDrawLine(dpy, client->frame, light_gc, 0, 0, width, 0);
  // light border left
  XDrawLine(dpy, client->frame, light_gc, 0, 1, 0, height - 1);
  
  // dark border top
  XDrawLine(dpy, client->frame, dark_gc, FRAME_BORDER_WIDTH, FRAME_TITLEBAR_HEIGHT - 1, width - FRAME_BORDER_WIDTH, FRAME_TITLEBAR_HEIGHT - 1);   
  // dark border right
  XDrawLine(dpy, client->frame, dark_gc, width - 1, 1, width - 1, height); 
  // dark border bottom
  XDrawLine(dpy, client->frame, dark_gc, 1, height - 1, width, height - 1);
  
  if (!client->shaded) {
  	// light border right
  	XDrawLine(dpy, client->frame, light_gc, width - FRAME_BORDER_WIDTH, FRAME_TITLEBAR_HEIGHT, width - FRAME_BORDER_WIDTH, height - FRAME_BORDER_WIDTH);
  
    // light border bottom
    XDrawLine(dpy, client->frame, light_gc, FRAME_BORDER_WIDTH, height - FRAME_BORDER_WIDTH, width - FRAME_BORDER_WIDTH, height - FRAME_BORDER_WIDTH);  

    // dark border left
    XDrawLine(dpy, client->frame, dark_gc, 3, FRAME_TITLEBAR_HEIGHT - 1, 3, height - FRAME_BORDER_WIDTH);
	  
    // lower left corder
    XDrawLine(dpy, client->frame, dark_gc, 0, height - FRAME_CORNER_OFFSET, FRAME_BORDER_WIDTH, height - FRAME_CORNER_OFFSET);
    XDrawLine(dpy, client->frame, light_gc, 0, height - FRAME_CORNER_OFFSET + 1, FRAME_BORDER_WIDTH, height - FRAME_CORNER_OFFSET + 1);

    XDrawLine(dpy, client->frame, dark_gc, FRAME_CORNER_OFFSET, height - FRAME_BORDER_WIDTH, FRAME_CORNER_OFFSET, height);
    XDrawLine(dpy, client->frame, light_gc, FRAME_CORNER_OFFSET + 1, height - FRAME_BORDER_WIDTH, FRAME_CORNER_OFFSET + 1, height);
  
    // lower right corner
    XDrawLine(dpy, client->frame, dark_gc, width - FRAME_CORNER_OFFSET, height - FRAME_BORDER_WIDTH, width - FRAME_CORNER_OFFSET, height);
    XDrawLine(dpy, client->frame, light_gc, width - FRAME_CORNER_OFFSET + 1, height - FRAME_BORDER_WIDTH, width - FRAME_CORNER_OFFSET + 1, height);
  
    XDrawLine(dpy, client->frame, dark_gc, width - FRAME_BORDER_WIDTH, height - FRAME_CORNER_OFFSET, width, height - FRAME_CORNER_OFFSET);  
    XDrawLine(dpy, client->frame, light_gc, width - FRAME_BORDER_WIDTH, height - FRAME_CORNER_OFFSET + 1, width, height - FRAME_CORNER_OFFSET + 1);
  }
  Rect initial_window = { .x = x, .y = y, .width = width, .height = height};
  draw_window_titlebar(client, initial_window);
  draw_close_button(client, initial_window);
}

Window create_titlebar_button(Window frame, int x, int y, int w, int h)
{
  Window button = XCreateSimpleWindow(dpy, frame, x, y, w, h, 0, 0x000000, 0x000000);				
  XSelectInput(dpy, button, ButtonMask);
			   	
  return button;
}

void frame(Window root, Window win) 
{
  XWindowAttributes attrs;
  XGetWindowAttributes(dpy, win, &attrs);
	
  Window frame = XCreateSimpleWindow(
                                     dpy,
                                     root,
                                     attrs.x,
                                     attrs.y,
                                     attrs.width + 10,
                                     attrs.height + 26,
                                     1,
                                     0x000000,
                                     0xaaaaaa);
		
  Window close_button = create_titlebar_button(frame, attrs.x + 3, attrs.y + 4, 13, 13);
			
  XSelectInput(
               dpy,
               frame,
               SubstructureRedirectMask | SubstructureNotifyMask | ButtonMask | ExposureMask | EnterWindowMask);
		 
  XAddToSaveSet(dpy, win);

  XReparentWindow(dpy, win, frame, 4, 20);  // Offset of client window within frame.
	 	
  Client *client = (Client *)malloc(sizeof *client);
  client->shaded = 0;
  client->client = win;
  client->frame = frame;
  client->close_button = close_button;
  client->xft_draw = XftDrawCreate(dpy, (Drawable) client->frame, DefaultVisual(dpy, DefaultScreen(dpy)), DefaultColormap(dpy, DefaultScreen(dpy)));
	 
  XFetchName(dpy, win, &client->title);
  ylist_ins_prev(&clients, ylist_head(&clients), client);
#ifdef DEBUG  
  print_client(client);
  fprintf(stderr, "Managed clients: %d\n", ylist_size(&clients));
#endif
  XMapWindow(dpy, frame);
  XMapWindow(dpy, close_button);
}

void unframe(Window win) 
{
#ifdef DEBUG
  fprintf(stderr, "unframe\n");
#endif
  Client *client = find_client(win);
  if (client != NULL) {
	XftDrawDestroy(client->xft_draw);
    XReparentWindow(dpy, client->client, root, 0, 0);
    XRemoveFromSaveSet(dpy, client->client);
	
    XUnmapWindow(dpy, client->close_button);
    XUnmapWindow(dpy, client->frame);
  }
}

void quit() 
{
  fprintf(stderr, "Quitting ywm...\n");
	
  free_menu();
  XFreeGC(dpy, focused_light_grey_gc);
  XFreeGC(dpy, focused_dark_grey_gc);
  XFreeGC(dpy, unfocused_light_grey_gc);
  XFreeGC(dpy, unfocused_dark_grey_gc);
  XFreeGC(dpy, focused_frame_gc);
  XFreeGC(dpy, unfocused_frame_gc);
  XFreeGC(dpy, light_red_gc);
  XFreeGC(dpy, dark_red_gc);
  XFreeGC(dpy, black_gc);
  XFreeCursor(dpy, pointerCursor);
  XFreeCursor(dpy, resize_v);
  XFreeCursor(dpy, resize_h);	

  ylist_destroy(&clients);
  ylist_destroy(&focus_stack);
  XCloseDisplay(dpy);
  exit(EXIT_SUCCESS);
}

void signal_handler(int signal) 
{
  pid_t pid;
  int status;
	
  switch(signal) 
    {
    case SIGTERM:
    case SIGINT:
      quit();
      break;
    case SIGHUP:
#ifdef DEBUG
  fprintf(stderr, "SIGHUP caught\n");
#endif
      break;
    case SIGCHLD:
#ifdef DEBUG
  fprintf(stderr, "SIGCHLD caught\n");
#endif
      while((pid = waitpid(-1, &status, WNOHANG)) != 0) {
        if ((pid == -1) && (errno != EINTR)) {
          break;
        } else {
          continue;
        }
      }
      break;
    }
}

int main(int argc, char *argv[])
{
#ifdef DEBUG
  fprintf(stderr, "DEBUG mode is on\n");
#endif

  XEvent ev;

  struct sigaction sigact;
  sigact.sa_handler = signal_handler;
  sigact.sa_flags = 0;
  sigaction(SIGTERM, &sigact, NULL);
  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGHUP, &sigact, NULL);
  sigaction(SIGCHLD, &sigact, NULL);	
  XSetErrorHandler(handle_xerror);
	
  setup_display();
  setup_wm_hints();

  ylist_init(&clients, free);
  ylist_init(&focus_stack, free);
  root_menu = create_menu();

  XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask | ButtonMask);

  while (True) {
    XNextEvent(dpy, &ev);
#ifdef DEBUG
    print_event(ev);
#endif
    switch(ev.type) {
    case KeyPress:
      on_key_press(&ev.xkey);
      break;
    case ButtonPress:
      on_button_press(&ev.xbutton);
      break;
    case ButtonRelease:
      on_button_release(&ev.xbutton);
      break;
    case MotionNotify:
      // Skip any already pending motion events.
      while (XCheckTypedWindowEvent(dpy, ev.xmotion.window, MotionNotify, &ev)) {}
      on_motion_notify(&ev.xmotion);
      break;
    case Expose:
      on_expose(&ev.xexpose);
      break;
    case CreateNotify:
      on_create_notify(&ev.xcreatewindow);
      break;
    case DestroyNotify:
      on_destroy_notify(&ev.xdestroywindow);
      break;
    case ReparentNotify:
      on_reparent_notify(&ev.xreparent);
      break;
    case ConfigureRequest:
      on_configure_request(&ev.xconfigurerequest);
      break;
    case ConfigureNotify:
      on_configure_notify(&ev.xconfigure);
      break;
    case MapRequest:
      on_map_request(root, &ev.xmaprequest);
      break;
    case UnmapNotify:
      on_unmap_notify(&ev.xunmap);
      break;
    case EnterNotify:
      on_enter_notify(&ev.xcrossing);
      break;
    case LeaveNotify:
      on_leave_notify(&ev.xcrossing);
      break;
    }
  }
}
