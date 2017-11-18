#include <stdlib.h>
#include <string.h>
#include <X11/cursorfont.h>
#include "menu.h"
#include "util.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int main()
{
    Display * dpy;
    Window root;
    XWindowAttributes attr;
    XButtonEvent start;
    XEvent ev;

    if(!(dpy = XOpenDisplay(0x0)))
    {
      return 1;
    }

    root = DefaultRootWindow(dpy);
    XColor backgroundColor = create_color(dpy, "#666797");
    XColor darker = create_color(dpy, "#555555");
    XColor lighter = create_color(dpy, "#D4D4D4");
    XColor lighter2 = create_color(dpy, "#ACACAC");
    XSetWindowBackground(dpy, root,  backgroundColor.pixel);
    XClearWindow(dpy, root);
    GC gc = XCreateGC(dpy, root, 0, NULL);
    /*
    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F1")), Mod1Mask, root,
            True, GrabModeAsync, GrabModeAsync);
    XGrabButton(dpy, 1, ControlMask, root, True, ButtonPressMask, GrabModeAsync,
            GrabModeAsync, None, None);
    XGrabButton(dpy, 3, ControlMask, root, True, ButtonPressMask, GrabModeAsync,
            GrabModeAsync, None, None);
    XGrabButton(dpy, 3, None, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
    */
    XDefineCursor(dpy, root, XCreateFontCursor(dpy, XC_left_ptr));
    
    int blackColor = BlackPixel(dpy, DefaultScreen(dpy));
    int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));
    Window menuWin = create_menu(dpy, gc, blackColor, whiteColor);
    //XSelectInput(dpy, menuWin, StructureNotifyMask);
    XSelectInput(dpy, root, ButtonPressMask | ButtonReleaseMask);
    XWindowAttributes menu_attr;
    int len = sizeof(menu_items) / sizeof(menu_items[0]);
    while (True) {
        XNextEvent(dpy, &ev);
        if(ev.type == KeyPress && ev.xkey.subwindow != None) {
            XRaiseWindow(dpy, ev.xkey.subwindow);
        } else if (ev.type == ButtonPress) {
	  if (ev.xbutton.state & ControlMask) {
	    XGrabPointer(dpy, ev.xbutton.subwindow, True,
			 PointerMotionMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
			 None, None, CurrentTime);

	    XGetWindowAttributes(dpy, ev.xbutton.subwindow, &attr);
	    start = ev.xbutton;
	  } else if (ev.xbutton.window == root && ev.xbutton.button == Button3 && ev.xbutton.subwindow == None) {
	    XGetWindowAttributes(dpy, menuWin, &menu_attr);
	    if (menu_attr.map_state == IsUnmapped) {
	        XMoveWindow(dpy, menuWin, ev.xbutton.x_root, ev.xbutton.y_root);
		XRaiseWindow(dpy, menuWin);
       	        XMapWindow(dpy, menuWin);
	    } else {
	        XUnmapWindow(dpy, menuWin);
	    }
	  } else if (menu_attr.map_state != IsUnmapped && ev.xbutton.button == Button1) {
	    int menu_index;
	    for (menu_index = 0; menu_index < len; menu_index++) {
	      if (menu_item_wins[menu_index] == ev.xbutton.window) {
		fork_exec(menu_items[menu_index].command);
	        XUnmapWindow(dpy, menuWin);
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
                attr.x + (start.button==1 ? xdiff : 0),
                attr.y + (start.button==1 ? ydiff : 0),
                MAX(1, attr.width + (start.button==3 ? xdiff : 0)),
                MAX(1, attr.height + (start.button==3 ? ydiff : 0)));
        } else if(ev.type == ButtonRelease) {
	    XUngrabPointer(dpy, CurrentTime);
	} else if (ev.type == Expose) {

	  XGetWindowAttributes(dpy, menuWin, &menu_attr);
	  if (menu_attr.map_state != IsUnmapped)
	    {	      
	      for (int i = 0; i < len; i++)
		{
		  Window menu_btn = menu_item_wins[i];
		  XWindowAttributes menu_item_attr;
		  XGetWindowAttributes(dpy, menu_btn, &menu_item_attr);
		  GC darkergc = XCreateGC(dpy, menu_btn, 0, NULL);
		  GC lightergc = XCreateGC(dpy, menu_btn, 0, NULL);
		  GC lightergc2 = XCreateGC(dpy, menu_btn, 0, NULL);
		  XSetForeground(dpy, darkergc, darker.pixel);
		  XSetForeground(dpy, lightergc, lighter.pixel);
		  XSetForeground(dpy, lightergc2, lighter2.pixel);

		  int width = menu_item_attr.width - 1;
		  int height = menu_item_attr.height - 1;
	        XDrawLine(dpy,
			  menu_btn,
			  lightergc,
			  0, 0, width, 0);
		XDrawLine(dpy,
			  menu_btn,
			  lightergc2,
			  1, 1, width, 1);
		XDrawLine(dpy,
			  menu_btn,
			  lightergc,
			  0, 1, 0, height);
		XDrawLine(dpy,
			  menu_btn,
			  lightergc2,
			  1, 1, 1, height);
		XDrawLine(dpy,
			  menu_btn,
			  darkergc,
			  0, height - 1, width, height - 1);
		XDrawLine(dpy,
			  menu_btn,
			  darkergc,
			  width - 1, 1, width - 1, height - 1);
		XDrawLine(dpy,
			  menu_btn,
			  DefaultGC(dpy, DefaultScreen(dpy)),
			  width, 0, width, height);
		XDrawLine(dpy,
			  menu_btn,
			  DefaultGC(dpy, DefaultScreen(dpy)),
			  0, height, width, height); 
 
		XDrawString(dpy,
			    menu_btn,
			    DefaultGC(dpy, DefaultScreen(dpy)),
			    7,
			    16,
			    menu_items[i].label,
			    strlen(menu_items[i].label));
	       }
	   
	    }
	}
    }
}
