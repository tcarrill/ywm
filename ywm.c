#include <stdlib.h>
#include <string.h>
#include <X11/cursorfont.h>
#include "menu.h"
#include "util.h"
#include "ywm.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static void setup_wm_hints() {
  atom_wm[AtomWMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
  atom_wm[AtomWMDeleteWindow] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
}

static void setup_display() {
  if(!(dpy = XOpenDisplay(0x0))) {
    exit(1);
  }

  root = DefaultRootWindow(dpy);

  XColor backgroundColor = create_color("#666797");
  XSetWindowBackground(dpy, root, backgroundColor.pixel);

  XClearWindow(dpy, root);
  XDefineCursor(dpy, root, XCreateFontCursor(dpy, XC_left_ptr));
}

int main()
{
    XWindowAttributes attr;
    XButtonEvent start;
    XEvent ev;

    setup_display();
    setup_wm_hints();

    Window menu_win = create_menu();

    XSelectInput(dpy, root, ButtonPressMask | ButtonReleaseMask);
    XWindowAttributes menu_attr;

    // TODO: refactor event handling, put in event.c file
    // delegate responsibility, i.e. menu events handled in menu.c
    while (True) {
        XNextEvent(dpy, &ev);
      	if(ev.type == KeyPress && ev.xkey.subwindow != None) {
            XRaiseWindow(dpy, ev.xkey.subwindow);
      	} else if (ev.type == ButtonPress) {
	  		if (ev.xbutton.state & ControlMask && ev.xbutton.subwindow != None) {
	  			if (ev.xbutton.button == 2) {
  				  send_wm_delete(ev.xbutton.subwindow);
	  			} else {
	  				XRaiseWindow(dpy, ev.xbutton.subwindow);
	  				XGrabPointer(dpy, ev.xbutton.subwindow, True,
			 		        PointerMotionMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
			 		            None, None, CurrentTime);

	    			XGetWindowAttributes(dpy, ev.xbutton.subwindow, &attr);
	    			start = ev.xbutton;
	  			}
			} else if (ev.xbutton.window == root && ev.xbutton.button == Button3 && ev.xbutton.subwindow == None) {
		    	XGetWindowAttributes(dpy, menu_win, &menu_attr);
		    	if (menu_attr.map_state == IsUnmapped) {
		        	XMoveWindow(dpy, menu_win, ev.xbutton.x_root, ev.xbutton.y_root);
					    XRaiseWindow(dpy, menu_win);
	       	    XMapWindow(dpy, menu_win);
		    	} else {
		        	XUnmapWindow(dpy, menu_win);
		    	}
			} else if (menu_attr.map_state != IsUnmapped && ev.xbutton.button == Button1) {
			    for (int i = 0; i < MENU_SIZE; i++) {
			         if (menu_item_wins[i] == ev.xbutton.window) {
					            fork_exec(menu_items[i].command);
			                XUnmapWindow(dpy, menu_win);
					            break;
			         }
			    }
	  		}
    	} else if(ev.type == MotionNotify) {
            int xdiff, ydiff;
            while(XCheckTypedEvent(dpy, MotionNotify, &ev));

            xdiff = ev.xbutton.x_root - start.x_root;
            ydiff = ev.xbutton.y_root - start.y_root;
            XMoveResizeWindow(dpy, ev.xmotion.window,
                attr.x + (start.button == 1 ? xdiff : 0),
                attr.y + (start.button == 1 ? ydiff : 0),
                MAX(1, attr.width + (start.button == 3 ? xdiff : 0)),
                MAX(1, attr.height + (start.button == 3 ? ydiff : 0)));
        } else if(ev.type == ButtonRelease) {
	    	XUngrabPointer(dpy, CurrentTime);
		} else if (ev.type == Expose) {
		  XGetWindowAttributes(dpy, menu_win, &menu_attr);

		  if (menu_attr.map_state != IsUnmapped) {
        draw_menu();
		  }
    }
  }
}
