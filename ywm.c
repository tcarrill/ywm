#include <string.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include "menu.h"
#include "util.h"
#include "ywm.h"
#include "event.h"

#define STRIP_LIGHT "#cacaca"
#define STRIP_DARK "#6a6a6a"

GC strip_light_gc;
GC strip_dark_gc;
	
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

  XClearWindow(dpy, root);
  XDefineCursor(dpy, root, XCreateFontCursor(dpy, XC_left_ptr));
}

void redraw(Display* dpy, Client *client) 
{ 
 XWindowAttributes frame_attrs;
 XGetWindowAttributes(dpy, client->frame, &frame_attrs);
 int yoffset = 3;
 for (int i = 0; i < 12; i++) {
 		 if (i % 2 == 0) {
 			 XDrawLine(dpy, client->frame, strip_light_gc, frame_attrs.x + 25, yoffset + i, frame_attrs.width - 25, yoffset + i);
 		 } else {
 			 XDrawLine(dpy, client->frame, strip_dark_gc, frame_attrs.x + 26, yoffset + i, frame_attrs.width - 24, yoffset + i);
 		 }
  }
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
	
 	 ylist_add_tail(clients, client);
 	 printf("Client {\n   client = %lu,\n   frame = %lu,\n   close_button = %lu\n}\n", client->client, client->frame, client->close_button);
	 fflush(stdout);
	 
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
