#ifndef EVENT_H
#define EVENT_H

#include <X11/Xlib.h>
#include <X11/cursorfont.h>


#define DBL_CLICK_SPEED 350
extern Time click1_time;
extern Time click2_time;

void on_key_press(const XKeyEvent* ev);
void on_button_press(const XButtonEvent* ev);
void on_button_release(const XButtonEvent* ev);
void on_motion_notify(const XMotionEvent* ev);
void on_expose(const XExposeEvent* ev);
void on_reparent_notify(const XReparentEvent* ev);
void on_create_notify(const XCreateWindowEvent* ev);
void on_destroy_notify(const XDestroyWindowEvent* ev);
void on_configure_request(const XConfigureRequestEvent* ev);
void on_configure_notify(const XConfigureEvent* ev);
void on_map_request(Window root, const XMapRequestEvent* ev);
void on_unmap_notify(const XUnmapEvent* ev);
void on_enter_notify(const XCrossingEvent* ev);
void on_leave_notify(const XCrossingEvent* ev);
#endif
