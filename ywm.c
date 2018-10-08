#include <string.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include "menu.h"
#include "util.h"
#include "ywm.h"
#include "event.h"

#define FRAME_COLOR "#aaaaaa"
#define STRIP_LIGHT "#cacaca"
#define STRIP_DARK "#6a6a6a"

GC strip_light_gc;
GC strip_dark_gc;
GC frame_gc;
	
static void setup_wm_hints() 
{
  atom_wm[AtomWMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
  atom_wm[AtomWMDeleteWindow] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
}

static void setup_display() 
{
  
  if (!(dpy = XOpenDisplay(0x0))) {
    exit(1);
  }

  root = DefaultRootWindow(dpy);
  printf("Root window: %lu\n", root);
  fflush(stdout);
  
  XColor backgroundColor = create_color("#666797");
  XSetWindowBackground(dpy, root, backgroundColor.pixel);

  title_font = XLoadQueryFont(dpy, DEF_FONT);
  if (title_font == NULL) {
  		printf("XLoadQueryFont(): font '%s' not found", DEF_FONT);
  		exit(1);
  }
  
  XGCValues gv;
  gv.function = GXcopy;
  gv.font = title_font->fid;
  gv.foreground = 0x000000;
  text_gc = XCreateGC(dpy, root, GCFunction|GCForeground|GCFont, &gv);

  XClearWindow(dpy, root);
  XDefineCursor(dpy, root, XCreateFontCursor(dpy, XC_left_ptr));
}

void redraw(Display* dpy, Client *client) 
{ 
 XWindowAttributes frame_attrs;
 XGetWindowAttributes(dpy, client->frame, &frame_attrs);
 
 int title_length = XTextWidth(title_font, client->title, strlen(client->title)); 	
 int titlex = (frame_attrs.width / 2) - (title_length / 2);
 
 int yoffset = 3;
 for (int i = 0; i < 12; i++) {
 		 if (i % 2 == 0) {
 			 XDrawLine(dpy, client->frame, strip_light_gc, frame_attrs.x + 22, yoffset + i, titlex - 10, yoffset + i);
			 XDrawLine(dpy, client->frame, strip_light_gc, titlex + title_length + 7, yoffset + i, frame_attrs.width - 30, yoffset + i);
 		 } else {
 			 XDrawLine(dpy, client->frame, strip_dark_gc, frame_attrs.x + 23, yoffset + i, titlex - 9, yoffset + i);
			 XDrawLine(dpy, client->frame, strip_dark_gc, titlex + title_length + 8, yoffset + i, frame_attrs.width - 29, yoffset + i);
 		 }
  }
  
  // light border
  XDrawLine(dpy, client->frame, strip_light_gc, 0, 0, frame_attrs.width, 0);
  XDrawLine(dpy, client->frame, strip_light_gc, 0, 1, 0, frame_attrs.height - 1);
  XDrawLine(dpy, client->frame, strip_light_gc, frame_attrs.width - 4, 20, frame_attrs.width - 4, frame_attrs.height - 4);
  XDrawLine(dpy, client->frame, strip_light_gc, 4, frame_attrs.height - 4, frame_attrs.width - 4, frame_attrs.height - 4);  
    
  // dark border
  XDrawLine(dpy, client->frame, strip_dark_gc, frame_attrs.width - 1, 1, frame_attrs.width - 1, frame_attrs.height);
  XDrawLine(dpy, client->frame, strip_dark_gc, 1, frame_attrs.height - 1, frame_attrs.width, frame_attrs.height - 1);
  XDrawLine(dpy, client->frame, strip_dark_gc, 3, 19, 3, frame_attrs.height - 4);
  XDrawLine(dpy, client->frame, strip_dark_gc, 4, 19, frame_attrs.width - 4, 19);  
  
  //XFillRectangle(dpy, client->frame, frame_gc, titlex - 10, 3, title_length + 20, 12);
  XDrawString(dpy, client->frame, text_gc, titlex, 13, client->title, strlen(client->title));
}

Window create_close_button(Display* dpy, Window win) 
{
	XWindowAttributes x_window_attrs;
	XGetWindowAttributes(dpy, win, &x_window_attrs);
    close_button = XCreateSimpleWindow(
         dpy,
         win,
         x_window_attrs.x + 5,
         x_window_attrs.y + 3,
         10,
         10,
         1,
         0x000000,
         0xfc5b57);
				
	XSelectInput(
	         dpy,
	         close_button,
	         ButtonPressMask | ButtonReleaseMask);
			   
	XMapWindow(dpy, close_button);
	
	return close_button;
}

void frame(Display* dpy, Window root, Window win) 
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
			
	XSelectInput(
         dpy,
         frame,
         SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask | ButtonReleaseMask | ExposureMask);
		 
	XAddToSaveSet(dpy, win);

    XReparentWindow(
         dpy,
         win,
         frame,
         4, 20);  // Offset of client window within frame.
	 
	 Window close_button = create_close_button(dpy, frame);
	
	 // TODO: map client window to frame with hashmap instead of list
	 Client *client = (Client *)malloc(sizeof *client);
	 client->client = win;
	 client->frame = frame;
	 client->close_button = close_button;
	 XFetchName(dpy, win, &client->title);
	 
 	 ylist_add_tail(clients, client);
 	 // printf("Client {\n   title = %s,\n   client = %lu,\n   frame = %lu,\n   close_button = %lu\n}\n", client->title, client->client, client->frame, client->close_button);
	 // fflush(stdout);
	 
	 XMapWindow(dpy, frame);
}

void unframe(Window w) 
{
	 
}

int main()
{
    XEvent ev;

    setup_display();
    setup_wm_hints();

    XGCValues gcv;
    gcv.function = GXcopy;

    gcv.foreground = create_color(STRIP_LIGHT).pixel;
    strip_light_gc = XCreateGC(dpy, root, GCFunction | GCForeground, &gcv);
 
    gcv.foreground = create_color(STRIP_DARK).pixel;
    strip_dark_gc = XCreateGC(dpy, root, GCFunction | GCForeground, &gcv);

    gcv.foreground = create_color(FRAME_COLOR).pixel;
    frame_gc = XCreateGC(dpy, root, GCFunction | GCForeground, &gcv);

	clients = ylist_new();
    // Window menu_win = create_menu();
    // XWindowAttributes menu_attr;

    XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask);

    while (True) {
        XNextEvent(dpy, &ev);
		printf("Received event: %d\n", ev.type);
      	
		switch(ev.type) {
			case KeyPress:
				on_key_press(dpy, &ev.xkey);
				break;
			case ButtonPress:
				on_button_press(dpy, &ev.xbutton);
				break;
			case ButtonRelease:
				on_button_release(dpy, &ev.xbutton);
				break;
			case MotionNotify:
				// Skip any already pending motion events.
	        	while (XCheckTypedWindowEvent(dpy, ev.xmotion.window, MotionNotify, &ev)) {}
				on_motion_notify(dpy, &ev.xmotion);
				break;
			case Expose:
				on_expose(dpy, &ev.xexpose);
				break;
			case CreateNotify:
				on_create_notify(dpy, &ev.xcreatewindow);
				break;
			case DestroyNotify:
				on_destroy_notify(dpy, &ev.xdestroywindow);
				break;
			case ReparentNotify:
				on_reparent_notify(dpy, &ev.xreparent);
				break;
			case ConfigureRequest:
				on_configure_request(dpy, &ev.xconfigurerequest);
				break;
			case MapRequest:
				on_map_request(dpy, root, &ev.xmaprequest);
				break;
			default:
				printf("Ignoring event\n");
		}
  }
  
  ylist_destroy(clients);
}
