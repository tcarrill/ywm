#include "event.h"
#define MAX(a, b) ((a) > (b) ? (a) : (b))
	
void on_key_press(Display* dpy, const XKeyEvent *ev) 
{
	// printf("\ton_key_press()\n");
	if (ev->subwindow != None) {
		XRaiseWindow(dpy, ev->subwindow);
	}
}

void on_button_press(Display* dpy, const XButtonEvent *ev)
{
	// printf("\ton_button_press()\n");
	// YNode* curr = clients->head;
	// Client* client = NULL;
	// printf("ev->window: %lu\n", ev->window);
	// while (curr != NULL) {
	// 	client = (Client *)curr->data;
	// 	printf("client->frame: %lu\n", client->frame);
	// 	if (client->frame == ev->window) {
	// 		printf("Found client!");
	// 		break;
	// 	}
	//
	// 	curr = curr->next;
	// }
	//
	// if (client == NULL) {
	// 	printf("!!! HEY - Could not find client in managed clients list!\n");
	// 	return;
	// }
	if (ev->window == root) {
		if (ev->button == Button3) {
			printf("Showing Application Menu\n");
			fflush(stdout);
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
	printf("ev->root: %lu\n", ev->root);
	printf("ev->window: %lu\n", ev->window);
	printf("ev->subwindow: %lu\n", ev->subwindow);
	fflush(stdout);
	
	if (ev->window == close_button) {
		printf("\tclose button pressed\n");
		fflush(stdout);
		send_wm_delete(ev->window);
	}
	XUngrabPointer(dpy, CurrentTime);
}

void on_motion_notify(Display* dpy, const XMotionEvent *ev)
{
	// printf("\ton_motion_notify()\n");

	int xdiff = ev->x_root - cursor_start_point.x;
	int ydiff = ev->y_root - cursor_start_point.y;

	if (ev->state & Button1Mask) {
		XMoveWindow(
		        dpy,
		        ev->window,
		        window_start.x + xdiff, 
				window_start.y + ydiff);
	} else if (ev->state & Button3Mask) {
		
	}
}

void on_expose(Display* dpy, const XExposeEvent *ev)
{
	// printf("\ton_expose()\n");
}

void on_reparent_notify(Display* dpy, const XReparentEvent *ev)
{
	// printf("\ton_reparent_notify()\n");
}

void on_create_notify(Display* dpy, const XCreateWindowEvent *ev)
{
	// printf("\ton_create_notify()\n");
}

void on_destroy_notify(Display* dpy, const XDestroyWindowEvent *ev)
{
	// printf("\ton_destroy_notify()\n");
}

void on_configure_request(Display* dpy, const XConfigureRequestEvent *ev)
{
	// printf("\ton_configure_request()\n");
	XWindowChanges changes;
	// Copy fields from e to changes.
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

void on_map_request(Display* dpy, Window root, const XMapRequestEvent* ev)
{
	// printf("\ton_map_request()\n");
    frame(dpy, root, ev->window);
    XMapWindow(dpy, ev->window);
}