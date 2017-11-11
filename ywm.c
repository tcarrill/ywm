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
    XColor backgroundColor = create_color(dpy, "#50526F");
    XSetWindowBackground(dpy, root,  backgroundColor.pixel);
    XClearWindow(dpy, root);
    GC gc = XCreateGC(dpy, root, 0, NULL);
    
    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F1")), Mod1Mask, root,
            True, GrabModeAsync, GrabModeAsync);
    XGrabButton(dpy, 1, ControlMask, root, True, ButtonPressMask, GrabModeAsync,
            GrabModeAsync, None, None);
    XGrabButton(dpy, 3, ControlMask, root, True, ButtonPressMask, GrabModeAsync,
            GrabModeAsync, None, None);
    XGrabButton(dpy, 3, None, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);

    XDefineCursor(dpy, root, XCreateFontCursor(dpy, XC_left_ptr));
    
    int blackColor = BlackPixel(dpy, DefaultScreen(dpy));
    int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));
    Window menuWin = create_menu(dpy, gc, blackColor, whiteColor);
    //XSelectInput(dpy, menuWin, StructureNotifyMask);
    XWindowAttributes menu_attr;
    while (True)
    {
        XNextEvent(dpy, &ev);
        if(ev.type == KeyPress && ev.xkey.subwindow != None)
	{
            XRaiseWindow(dpy, ev.xkey.subwindow);
        }
	else if (ev.type == ButtonPress && ev.xbutton.subwindow == None)
	{
	    XGetWindowAttributes(dpy, menuWin, &menu_attr);
	    if (menu_attr.map_state == IsUnmapped)
	    {
	        XMoveWindow(dpy, menuWin, ev.xbutton.x_root, ev.xbutton.y_root);
		XRaiseWindow(dpy, menuWin);
       	        XMapWindow(dpy, menuWin);
	    }
	    else
	    {
	        XUnmapWindow(dpy, menuWin);
	    }
	}
	else if(ev.type == ButtonPress && ev.xbutton.subwindow != None)
        {
	  printf("%lx - %lx\n", ev.xbutton.window, ev.xbutton.subwindow);
	  if (ev.xbutton.window == menu_item_wins[0])
	    {
	      printf("%s\n", menu_items[0].label);
	    }
	  else
	    {
	  
            XGrabPointer(dpy, ev.xbutton.subwindow, True,
                    PointerMotionMask|ButtonReleaseMask, GrabModeAsync,
                    GrabModeAsync, None, None, CurrentTime);

            XGetWindowAttributes(dpy, ev.xbutton.subwindow, &attr);
            start = ev.xbutton;
	    }
        }
        else if(ev.type == MotionNotify)
        {
            int xdiff, ydiff;
            while(XCheckTypedEvent(dpy, MotionNotify, &ev));

            xdiff = ev.xbutton.x_root - start.x_root;
            ydiff = ev.xbutton.y_root - start.y_root;
            XMoveResizeWindow(dpy, ev.xmotion.window,
                attr.x + (start.button==1 ? xdiff : 0),
                attr.y + (start.button==1 ? ydiff : 0),
                MAX(1, attr.width + (start.button==3 ? xdiff : 0)),
                MAX(1, attr.height + (start.button==3 ? ydiff : 0)));
        }
        else if(ev.type == ButtonRelease)
	{
	    XUngrabPointer(dpy, CurrentTime);
	}
	else if (ev.type == Expose)
	{
	  XGetWindowAttributes(dpy, menuWin, &menu_attr);
	  if (menu_attr.map_state != IsUnmapped)
	    {
	      int len = sizeof(menu_items) / sizeof(menu_items[0]);
	      
	      for (int i = 0; i < len; i++)
		{
		XWindowAttributes menu_item_attr;
		XGetWindowAttributes(dpy, menu_item_wins[i], &menu_item_attr);
                		
		XDrawString(dpy,
			    menu_item_wins[i],
			    DefaultGC(dpy, DefaultScreen(dpy)),
			    5,
			    17,
			    menu_items[i].label,
			    strlen(menu_items[i].label));
	       }
	    }
	}
    }
}
