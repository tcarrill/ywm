#include "event.h"
#define MAX(a, b) ((a) > (b) ? (a) : (b))

Point prev_mouse_xy;

void on_key_press(const XKeyEvent *ev) 
{
  if (ev->subwindow != None) {
    XRaiseWindow(dpy, ev->subwindow);
  }
}

void on_button_press(const XButtonEvent *ev)
{
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
    cursor_start_win_point = (Point){ .x = ev->x, .y = ev->y };	
    prev_mouse_xy = (Point){ .x = ev->x_root, .y = ev->y_root };
    start_window_geom = (Rect){ .x = x, .y = y, .width = width, .height = height };
    current_window_geom = (Rect){ .x = x, .y = y, .width = width, .height = height };

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
  Client *c = find_client_by_type(ev->window, CLOSE_BTN);
  if (c != NULL) {
    send_wm_delete(c->client);	
    remove_client(c);
  }

  XUngrabPointer(dpy, CurrentTime);
}

void on_motion_notify(const XMotionEvent *ev)
{
  Client *c = find_client(ev->window);

  int x = current_window_geom.x;
  int y = current_window_geom.y;
  int width = current_window_geom.width;
  int height = current_window_geom.height;
  
  if (ev->state & Button1Mask && c->close_button != ev->window && !is_resize_frame(cursor_start_win_point)) {
    int xdiff = ev->x_root - cursor_start_point.x;
    int ydiff = ev->y_root - cursor_start_point.y;

    x = start_window_geom.x + xdiff;
    y = start_window_geom.y + ydiff;
		
    if (snap_window_right(x)) { 
      x = screen_w - start_window_geom.width;		
    } else if (snap_window_left(x)) {	
      x = 0;
    } 
		
    if (snap_window_bottom(y)) { 
      y = screen_h - start_window_geom.height;
    } else if (snap_window_top(y)) {
      y = 0;
    }
		
    XMoveWindow(dpy, ev->window, x, y);
  } else { // resize motion        
    if (is_lower_right_corner(cursor_start_win_point)) {
      int xdiff = ev->x_root - cursor_start_point.x;
      int ydiff = ev->y_root - cursor_start_point.y;
      width = start_window_geom.width + xdiff;
      height = start_window_geom.height + ydiff;
      
      XResizeWindow(dpy, c->frame, width + 10, height + 26);
      XResizeWindow(dpy, c->client, width, height);  
    } else {
      int xdiff = ev->x_root - prev_mouse_xy.x;
      int ydiff = ev->y_root - prev_mouse_xy.y;
      if (is_lower_left_corner(cursor_start_win_point)) {
        x = current_window_geom.x + xdiff;
        width = current_window_geom.width + (xdiff * -1);
        height = current_window_geom.height + ydiff;
      } else if (is_left_frame(cursor_start_win_point.x)) {
        x = current_window_geom.x + xdiff;
        width = current_window_geom.width + (xdiff * -1);
      } else if (is_right_frame(cursor_start_win_point.x)) {
        width = current_window_geom.width + xdiff;
      } else if (is_bottom_frame(cursor_start_win_point.y)) {
        height = current_window_geom.height + ydiff;
      }

      XMoveResizeWindow(dpy, c->frame, x, y, width, height);
      XMoveResizeWindow(dpy, c->client, FRAME_BORDER_WIDTH, FRAME_TITLEBAR_HEIGHT, width - 10, height - 26);
    }
  }

  current_window_geom = (Rect){ .x = x, .y = y, .width = width, .height = height };
  prev_mouse_xy = (Point){ .x = ev->x_root, .y = ev->y_root };
}

void on_expose(const XExposeEvent *ev)
{
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
}

void on_create_notify(const XCreateWindowEvent *ev)
{
}

void on_destroy_notify(const XDestroyWindowEvent *ev)
{
  Client *c = find_client(ev->window);
  if (c != NULL) {
    remove_client(c);
  }
}

void on_configure_request(const XConfigureRequestEvent *ev)
{
  XWindowChanges changes;
  changes.x = ev->x;
  changes.y = ev->y;
  changes.width = ev->width;
  changes.height = ev->height;
  changes.border_width = ev->border_width;
  changes.sibling = ev->above;
  changes.stack_mode = ev->detail;
  XConfigureWindow(dpy, ev->window, ev->value_mask, &changes);
  //Client* c = find_client(ev->window);
  //if (c != NULL) {
  //  redraw(c);
  //}
}

void on_configure_notify(const XConfigureEvent* ev)
{
}

void on_map_request(Window root, const XMapRequestEvent* ev)
{
  Client *c = find_client(ev->window);
  if (c == NULL) {
    frame(root, ev->window);
    XMapWindow(dpy, ev->window);
  }
}

void on_unmap_notify(const XUnmapEvent* ev) 
{
  // printf("\ton_unmap_notify()\n");
  // Client *c = find_client(ev->window);
  // print_client(c);
  // if (c != NULL) {
  // 		remove_client(dpy, c);
  // 	}
}

void on_enter_notify(const XCrossingEvent* ev)
{
  focused_client = find_client(ev->window);
  /*
  if (ev->window != root) { 
    if (is_left_frame(ev->x)) {
      XDefineCursor(dpy, ev->window, resize_h);
    } else if (is_right_frame(ev->x)) {
      XDefineCursor(dpy, ev->window, resize_h);  
    } else if (is_bottom_frame(ev->y)) {
      XDefineCursor(dpy, ev->window, resize_v);
    } else if (is_top_frame(ev->y)) {
      XDefineCursor(dpy, ev->window, resize_v);
    } else {
      XDefineCursor(dpy, ev->window, pointer);
    }
  }
  */
	
  YNode *curr = ylist_head(&clients);
  while (curr != NULL) {
    Client *client = (Client *)ylist_data(curr);
    redraw(client);
    curr = curr->next;
  }
}
void on_leave_notify(const XCrossingEvent* ev) 
{
/*
  if (is_left_frame(ev->x)) {
    XDefineCursor(dpy, ev->window, resize_h);
  } else if (is_right_frame(ev->x)) {
    XDefineCursor(dpy, ev->window, resize_h);  
  } else if (is_bottom_frame(ev->y)) {
    XDefineCursor(dpy, ev->window, resize_v);
  } else if (is_top_frame(ev->y)) {
    XDefineCursor(dpy, ev->window, resize_v);
  }
*/ 
}
