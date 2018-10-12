#ifndef EVENT_H
#define EVENT_H

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include "ywm.h"
#include "client.h"
#include "util.h"

void on_key_press(Display* dpy, const XKeyEvent* ev);
void on_button_press(Display* dpy, const XButtonEvent* ev);
void on_button_release(Display* dpy, const XButtonEvent* ev);
void on_motion_notify(Display* dpy, const XMotionEvent* ev);
void on_expose(Display* dpy, const XExposeEvent* ev);
void on_reparent_notify(Display* dpy, const XReparentEvent* ev);
void on_create_notify(Display* dpy, const XCreateWindowEvent* ev);
void on_destroy_notify(Display* dpy, const XDestroyWindowEvent* ev);
void on_configure_request(Display* dpy, const XConfigureRequestEvent* ev);
void on_configure_notify(Display* dpy, const XConfigureEvent* ev);
void on_map_request(Display* dpy, Window root, const XMapRequestEvent* ev);
void on_unmap_notify(Display* dpy, const XUnmapEvent* ev);
#endif