#include <string.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include "menu.h"
#include "util.h"
#include "ywm.h"
#include "event.h"

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

void draw_close_button(Display* dpy, Window win) 
{
	XWindowAttributes x_window_attrs;
	XGetWindowAttributes(dpy, win, &x_window_attrs);
    close_button = XCreateSimpleWindow(
         dpy,
         win,
         x_window_attrs.x + 5,
         x_window_attrs.y + 5,
         10,
         10,
         1,
         0x000000,
         0xdf2020);
				
	XSelectInput(
	         dpy,
	         close_button,
	         ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
			   
	XMapWindow(dpy, close_button);
		
	printf("Close Button: %lu\n", close_button);
	printf("    on frame: %lu\n", win);
	fflush(stdout);	 
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
         SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask | ButtonReleaseMask);
		 
	XAddToSaveSet(dpy, win);

    XReparentWindow(
         dpy,
         win,
         frame,
         4, 20);  // Offset of client window within frame.

	 // TODO: map client window to frame with hashmap instead of list
	 // Client *client = &(Client){ .client = win, .frame = frame };
 // 	 ylist_add_tail(clients, client);
 // 	 printf("Added client: %lu with frame: %lu\n", client->client, client->frame);
 // 	 printf("Clients managed: %d\n", clients->length);
	 draw_close_button(dpy, frame);
	 XMapWindow(dpy, frame);
	
	printf("Frame: %lu\n", frame);
	printf("Client: %lu\n", win);
	fflush(stdout);
	 // XGrabButton(
	 //       dpy,
	 //       Button1,
	 //       None,
	 //       frame,
	 //       False,
	 //       ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
	 //       GrabModeAsync,
	 //       GrabModeAsync,
	 //       None,
	 //       None);
	 //   //   b. Resize windows with alt + right button.
	 //  XGrabButton(
	 //       dpy,
	 //       Button3,
	 //       None,
	 //       frame,
	 //       False,
	 //       ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
	 //       GrabModeAsync,
	 //       GrabModeAsync,
	 //       None,
	 //       None);
}

int main()
{
    XEvent ev;

    setup_display();
    setup_wm_hints();

	// clients = ylist_new();
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
}
