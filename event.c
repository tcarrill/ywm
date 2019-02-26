#include "event.h"
#define MAX(a, b) ((a) > (b) ? (a) : (b))
	
void on_key_press(const XKeyEvent *ev) 
{
#ifdef DEBUG
  fprintf(stderr, "\ton_key_press()\n");
#endif
  if (ev->subwindow != None) {
    XRaiseWindow(dpy, ev->subwindow);
  }
}

void on_button_press(const XButtonEvent *ev)
{
#ifdef DEBUG
  fprintf(stderr, "\ton_button_press()\n");
#endif
  XWindowAttributes menu_attr;
  XGetWindowAttributes(dpy, root_menu, &menu_attr);
  if (ev->window == root  && ev->subwindow == None) {
    if (ev->button == Button3) {
      if (menu_attr.map_state == IsUnmapped) {
        XMoveWindow(dpy, root_menu, ev->x_root, ev->y_root);	
        XRaiseWindow(dpy, root_menu);
        XMapWindow(dpy, root_menu);
      } else {
        XUnmapWindow(dpy, root_menu);
      }
    } else {
      XUnmapWindow(dpy, root_menu);
    }
  } else if (menu_attr.map_state != IsUnmapped && ev->button == Button1) {
    YNode *curr = ylist_head(&menu_items);
    while (curr != NULL) {
      MenuItem *menuItem = (MenuItem *)curr->data;
      if (menuItem->window == ev->window) {
        flash_menu(menuItem);      
        fork_exec(menuItem->command);
        XUnmapWindow(dpy, root_menu);
        break;
      }
      curr = curr->next;
    }
  } else {
    XAllowEvents(dpy, ReplayPointer, CurrentTime);
		
    Window returned_root;
    int x, y;
    unsigned width, height, border_width, depth;
    XGetGeometry(
                 dpy,
                 ev->window,
                 &returned_root,
                 &x, &y,
                 &width, &height,
                 &border_width,
                 &depth);
			
    cursor_start_point = (Point){ .x = ev->x_root, .y = ev->y_root };	
    window_start = (Rect){ .x = x, .y = y, .width = width, .height = height };

    XGrabPointer(dpy, 
                 ev->window, 
                 True,
                 PointerMotionMask | ButtonReleaseMask, 
                 GrabModeAsync, 
                 GrabModeAsync,
                 None, 
                 None, 
                 CurrentTime);

    XRaiseWindow(dpy, ev->window);
    focused_client = find_client(ev->window);
		
    YNode *curr = ylist_head(&clients);
    while (curr != NULL) {
      Client *client = (Client *)ylist_data(curr);
      redraw(client);
      curr = curr->next;
    }
  }
}

void on_button_release(const XButtonEvent *ev)
{
#ifdef DEBUG
  fprintf(stderr, "\ton_button_release()\n");
#endif
  Client *c = find_client_by_type(ev->window, CLOSE_BTN);
  if (c != NULL) {
    send_wm_delete(c->client);	
    remove_client(c);
  }

  XUngrabPointer(dpy, CurrentTime);
}

void on_motion_notify(const XMotionEvent *ev)
{
#ifdef DEBUG
  fprintf(stderr, "\ton_motion_notify()\n");
#endif
  int xdiff = ev->x_root - cursor_start_point.x;
  int ydiff = ev->y_root - cursor_start_point.y;
  Client *c = find_client(ev->window);
	
  if (ev->state & Button1Mask && c->close_button != ev->window) {
    int x = window_start.x + xdiff;
    int y = window_start.y + ydiff;
		
    if (snap_window_right(x)) { 
      x = screen_w - window_start.width;		
    } else if (snap_window_left(x)) {	
      x = 0;
    } 
		
    if (snap_window_bottom(y)) { 
      y = screen_h - window_start.height;
    } else if (snap_window_top(y)) {
      y = 0;
    }
		
    XMoveWindow(dpy, ev->window, x, y);
  } else if (ev->state & Button3Mask) {
    int width = window_start.width + xdiff;
    int height = window_start.height + ydiff;
		
    if (width > 25 && height > 10) {
      XResizeWindow(dpy, c->frame, width + 10, height + 26);
      XResizeWindow(dpy, c->client, width, height);
    }
  }
}

void on_expose(const XExposeEvent *ev)
{
#ifdef DEBUG
  fprintf(stderr, "\ton_expose()\n");
#endif
  if (ev->count == 0) {
    Client* c = find_client(ev->window);
    if (c != NULL) {
      redraw(c);
    } 
		
    XWindowAttributes menu_attr;
    XGetWindowAttributes(dpy, root_menu, &menu_attr);
    if (menu_attr.map_state != IsUnmapped) {
      draw_menu();
    }
  }	
}

void on_reparent_notify(const XReparentEvent *ev)
{
#ifdef DEBUG
  fprintf(stderr, "\ton_reparent_notify()\n");
#endif
}

void on_create_notify(const XCreateWindowEvent *ev)
{
#ifdef DEBUG
  fprintf(stderr, "\ton_create_notify()\n");
#endif
}

void on_destroy_notify(const XDestroyWindowEvent *ev)
{
#ifdef DEBUG
  fprintf(stderr, "\ton_destroy_notify(%#lx)\n", ev->window);
#endif
  Client *c = find_client(ev->window);
  if (c != NULL) {
    remove_client(c);
  }
}

void on_configure_request(const XConfigureRequestEvent *ev)
{
#ifdef DEBUG
  fprintf(stderr, "\ton_configure_request()\n");
#endif
  XWindowChanges changes;
  changes.x = ev->x;
  changes.y = ev->y;
  changes.width = ev->width;
  changes.height = ev->height;
  changes.border_width = ev->border_width;
  changes.sibling = ev->above;
  changes.stack_mode = ev->detail;
  XConfigureWindow(dpy, ev->window, ev->value_mask, &changes);
  Client* c = find_client(ev->window);
  if (c != NULL) {
    redraw(c);
  }
}

void on_configure_notify(const XConfigureEvent* ev)
{
#ifdef DEBUG
  fprintf(stderr, "\ton_configure_notify()\n");
#endif
}

void on_map_request(Window root, const XMapRequestEvent* ev)
{
#ifdef DEBUG
  fprintf(stderr, "\ton_map_request()\n");
#endif
  Client *c = find_client(ev->window);
  if (c == NULL) {
    frame(root, ev->window);
    XMapWindow(dpy, ev->window);
  }
}

void on_unmap_notify(const XUnmapEvent* ev) 
{
#ifdef DEBUG
  fprintf(stderr, "\ton_unmap_notify()\n");
#endif
  // printf("\ton_unmap_notify()\n");
  // Client *c = find_client(ev->window);
  // print_client(c);
  // if (c != NULL) {
  // 		remove_client(dpy, c);
  // 	}
}

void on_enter_notify(const XCrossingEvent* ev)
{
#ifdef DEBUG
  fprintf(stderr, "\ton_enter_notify()\n");
#endif
  focused_client = find_client(ev->window);
	
  YNode *curr = ylist_head(&clients);
  while (curr != NULL) {
    Client *client = (Client *)ylist_data(curr);
    redraw(client);
    curr = curr->next;
  }
}
