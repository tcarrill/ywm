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

void draw_window_titlebar(Display* dpy, Client *client, Rect initial_window) 
{ 	
    int yoffset = 4;
	int left_xstart_light = 20;
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
	    int title_width = XTextWidth(title_font, client->title, title_len); 	
	    int titlex = (initial_window.width / 2) - (title_width / 2);
	    XDrawString(dpy, client->frame, text_gc, titlex, 14, client->title, title_len);
			
		if (client == focused_client) {
			int left_xend_light = titlex - 10;
			int right_xstart_light = titlex + title_width + 7;
			int right_xend_light = initial_window.width - 5;
		
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
		}   
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

void redraw(Display* dpy, Client *client) 
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
  
  // light border
  // top
  XDrawLine(dpy, client->frame, light_gc, 0, 0, width, 0);
  // left
  XDrawLine(dpy, client->frame, light_gc, 0, 1, 0, height - 1);
  // right
  XDrawLine(dpy, client->frame, light_gc, width - FRAME_BORDER_WIDTH, FRAME_TITLEBAR_HEIGHT, width - FRAME_BORDER_WIDTH, height - FRAME_BORDER_WIDTH);
  // bottom
  XDrawLine(dpy, client->frame, light_gc, FRAME_BORDER_WIDTH, height - FRAME_BORDER_WIDTH, width - FRAME_BORDER_WIDTH, height - FRAME_BORDER_WIDTH);  
    
  // dark border
  // top
  XDrawLine(dpy, client->frame, dark_gc, FRAME_BORDER_WIDTH, FRAME_TITLEBAR_HEIGHT - 1, width - FRAME_BORDER_WIDTH, FRAME_TITLEBAR_HEIGHT - 1); 
  // left
  XDrawLine(dpy, client->frame, dark_gc, 3, FRAME_TITLEBAR_HEIGHT - 1, 3, height - FRAME_BORDER_WIDTH);
  // right
  XDrawLine(dpy, client->frame, dark_gc, width - 1, 1, width - 1, height); 
  // bottom
  XDrawLine(dpy, client->frame, dark_gc, 1, height - 1, width, height - 1);
  
  Rect initial_window = { .x = x, .y = y, .width = width, .height = height};
  draw_window_titlebar(dpy, client, initial_window);
}

Window create_close_button(Display* dpy, Window frame) 
{
	XWindowAttributes x_window_attrs;
	XGetWindowAttributes(dpy, frame, &x_window_attrs);
    Window close_button = XCreateSimpleWindow(
         dpy,
         frame,
         x_window_attrs.x + 5,
         x_window_attrs.y + 4,
         10,
         10,
         1,
         0x000000,
         create_color(RED).pixel);
				
	XSelectInput(
	         dpy,
	         close_button,
	         ButtonPressMask | ButtonReleaseMask);
			   	
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
		
	Window close_button = create_close_button(dpy, frame);
			
	XSelectInput(
         dpy,
         frame,
         SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask | ButtonReleaseMask | ExposureMask | EnterWindowMask);
		 
	XAddToSaveSet(dpy, win);

    XReparentWindow(
         dpy,
         win,
         frame,
         4, 20);  // Offset of client window within frame.
	 	
	 // TODO: map client window to frame with hashmap instead of list
	 Client *client = (Client *)malloc(sizeof *client);
	 client->client = win;
	 client->frame = frame;
	 client->close_button = close_button;
	 XFetchName(dpy, win, &client->title);
	 print_client(client);
	 ylist_ins_prev(&clients, ylist_head(&clients), client);
	 printf("Managed clients: %d\n", ylist_size(&clients));
	 fflush(stdout);
	 XMapWindow(dpy, frame);
 	 XMapWindow(dpy, close_button);
}

void unframe(Display* dpy, Window win) 
{
	Client *client = find_client(win);
	if (client == NULL) {
		printf("NO CLIENT FOUND!\n");
	    fflush(stdout);
		return;
	}
	XReparentWindow(dpy, client->client, root, 0, 0);
	XRemoveFromSaveSet(dpy, client->client);
	
	XUnmapWindow(dpy, client->close_button);
 	XUnmapWindow(dpy, client->frame);

	// XDestroyWindow(dpy, client->close_button);
	// XDestroyWindow(dpy, client->frame);
}

int main()
{
    XEvent ev;

    setup_display();
    setup_wm_hints();

    XGCValues gcv;
    gcv.function = GXcopy;

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

	ylist_init(&clients, free);
    ylist_init(&focus_stack, free);
	root_menu = create_menu();

    XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask | ButtonReleaseMask);

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
			case ConfigureNotify:
				on_configure_notify(dpy, &ev.xconfigure);
				break;
			case MapRequest:
				on_map_request(dpy, root, &ev.xmaprequest);
				break;
			case UnmapNotify:
				on_unmap_notify(dpy, &ev.xunmap);
				break;
			case EnterNotify:
				on_enter_notify(dpy, &ev.xcrossing);
				break;
			default:
				printf("\tIgnoring event\n");
		}
  }
  
  ylist_destroy(&clients);
  ylist_destroy(&focus_stack);
}
