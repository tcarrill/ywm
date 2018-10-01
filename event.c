#include "event.h"
	
void on_key_press(Display* dpy, const XKeyEvent *ev) 
{
	printf("\ton_key_press()\n");
	if (ev->subwindow != None) {
		XRaiseWindow(dpy, ev->subwindow);
	}
}

void on_button_press(Display* dpy, const XButtonEvent *ev)
{
	printf("\ton_button_press()\n");
}

void on_button_release(Display* dpy, const XButtonEvent *ev)
{
	printf("\ton_button_release()\n");
}

void on_motion_notify(Display* dpy, const XMotionEvent *ev)
{
	printf("\ton_motion_notify()\n");
}

void on_expose(Display* dpy, const XExposeEvent *ev)
{
	printf("\ton_expose()\n");
}

void on_reparent_notify(Display* dpy, const XReparentEvent *ev)
{
	printf("\ton_reparent_notify()\n");
}

void on_create_notify(Display* dpy, const XCreateWindowEvent *ev)
{
	printf("\ton_create_notify()\n");
}

void on_destroy_notify(Display* dpy, const XDestroyWindowEvent *ev)
{
	printf("\ton_destroy_notify()\n");
}

void on_configure_request(Display* dpy, const XConfigureRequestEvent *ev)
{
	printf("\ton_configure_request()\n");
}

void on_map_request(Display* dpy, Window root, const XMapRequestEvent* ev)
{
	printf("\ton_map_request()\n");
    frame(dpy, root, ev->window);
    XMapWindow(dpy, ev->window);
}