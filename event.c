#include "event.h"
#define MAX(a, b) ((a) > (b) ? (a) : (b))
	
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
		for (int i = 0; i < MENU_SIZE; i++) {
			if (menu_item_wins[i] == ev->window) {
                 flash_menu(i);
				 fork_exec(menu_items[i].command);
			     XUnmapWindow(dpy, root_menu);
			     break;
		    }
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
	}
}

void on_button_release(Display* dpy, const XButtonEvent *ev)
{
	printf("\ton_button_release()\n");
	
	Client *c = find_client_by_type(ev->window, CLOSE_BTN);
	if (c != NULL) {
		send_wm_delete(c->client);	
		remove_client(dpy, c);
	}
	
	XUngrabPointer(dpy, CurrentTime);
}

void on_motion_notify(Display* dpy, const XMotionEvent *ev)
{
	// printf("\ton_motion_notify()\n");
	int xdiff = ev->x_root - cursor_start_point.x;
	int ydiff = ev->y_root - cursor_start_point.y;
	Client *c = find_client(ev->window);
	
	if (ev->state & Button1Mask && c->close_button != ev->window) {
		XMoveWindow(
		        dpy,
		        ev->window,
		        window_start.x + xdiff, 
				window_start.y + ydiff);
	} else if (ev->state & Button3Mask) {
		XResizeWindow(dpy, c->frame, window_start.width + xdiff + 10, window_start.height + ydiff + 26);
		XResizeWindow(dpy, c->client, MAX(1, window_start.width + xdiff), MAX(1, window_start.height + ydiff));
	}
}

void on_expose(Display* dpy, const XExposeEvent *ev)
{
	// printf("\ton_expose()\n");
	// printf("\tev->window %lu\n", ev->window);
	// fflush(stdout);
	if (ev->count == 0) {
		Client* c = find_client(ev->window);
		if (c != NULL) {
			redraw(dpy, c);
		} 
		
		XWindowAttributes menu_attr;
		XGetWindowAttributes(dpy, root_menu, &menu_attr);
		if (menu_attr.map_state != IsUnmapped) {
			draw_menu();
		}
	}	
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
	Client *c = find_client(ev->window);
	if (c != NULL) {
		remove_client(dpy, c);
	} else {
		printf("\tCould not find client containing window %lu\n", ev->window);
	}
}

void on_configure_request(Display* dpy, const XConfigureRequestEvent *ev)
{
	printf("\ton_configure_request()\n");
	XWindowChanges changes;
	changes.x = ev->x;
	changes.y = ev->y;
	changes.width = ev->width;
	changes.height = ev->height;
	changes.border_width = ev->border_width;
	changes.sibling = ev->above;
	changes.stack_mode = ev->detail;
	// Grant request by calling XConfigureWindow().
	XConfigureWindow(dpy, ev->window, ev->value_mask, &changes);
}

void on_configure_notify(Display* dpy, const XConfigureEvent* ev)
{
	printf("\ton_configure_notify()\n");
}

void on_map_request(Display* dpy, Window root, const XMapRequestEvent* ev)
{
	printf("\ton_map_request()\n");
	Client *c = find_client(ev->window);
	if (c == NULL) {
	    frame(dpy, root, ev->window);
	    XMapWindow(dpy, ev->window);
	}
}

void on_unmap_notify(Display* dpy, const XUnmapEvent* ev) 
{
	printf("\ton_unmap_notify()\n");
	
	if (ev->event == root) {
		printf("\tIgnoring unmap!\n");
		return;
	}
	
	Client *c = find_client(ev->window);
	if (c == NULL) {
		printf("\tCould not find client containing window %lu\n", ev->window);
		return;
	}
	printf("\tev->window: %lu\n", ev->window);
	print_client(c);
	
	// remove_client(dpy, c);
	// unframe(dpy, ev->window);
}