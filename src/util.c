#include "util.h"
#include <string.h>
#include <limits.h>     /* PATH_MAX */
#include <sys/stat.h>   /* mkdir(2) */
#include <errno.h>

int is_above(Rect a, Rect b) 
{
	if (a.y + a.height < b.y) {
		return 1;
	}
	
	return 0;
}

int is_below(Rect a, Rect b) 
{
	if (a.y > b.y + b.height) {
		return 1;
	}
	
	return 0;
}

int is_left(Rect a, Rect b)
{
	if (a.x + a.width < b.x) {
		return 1;
	}
	
	return 0;
}

int is_right(Rect a, Rect b) 
{
	if (a.x > b.x + b.width) {
		return 1;
	}
	
	return 0;
}

int intersect(Rect a, Rect b)
{
	int a_right_edge = a.x + a.width;
	int b_right_edge = b.x + b.width;
	int a_bottom_edge = a.y + a.height;
	int b_bottom_edge = b.y + b.height;
	
	if ((a_right_edge > b.x - SNAP_BUFFER) && (a_right_edge < b.x) && !is_above(a, b) && !is_below(a, b)) {
		return SNAP_LEFT;
	} else if ((a.x < b_right_edge + SNAP_BUFFER) && (a.x > b_right_edge) && !is_above(a, b) && !is_below(a, b)) {
		return SNAP_RIGHT; 
	} else if ((a_bottom_edge > b.y - SNAP_BUFFER) && (a_bottom_edge < b.y) && !is_left(a, b) && !is_right(a, b)) { 
		return SNAP_TOP;
	} else if ((a.y < b_bottom_edge + SNAP_BUFFER) && (a.y > b_bottom_edge) && !is_left(a, b) && !is_right(a, b)) {
		return SNAP_BOTTOM;
	} else {
		return 0;
	}
}

int mkdir_p(const char *path)
{
    /* Adapted from http://stackoverflow.com/a/2336245/119527 */
    const size_t len = strlen(path);
    char _path[PATH_MAX];
    char *p; 

    errno = 0;

    /* Copy string so its mutable */
    if (len > sizeof(_path)-1) {
        errno = ENAMETOOLONG;
        return -1; 
    }   
    strcpy(_path, path);

    /* Iterate the string */
    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
            /* Temporarily truncate */
            *p = '\0';

            if (mkdir(_path, S_IRWXU) != 0) {
                if (errno != EEXIST)
                    return -1; 
            }

            *p = '/';
        }
    }   

    if (mkdir(_path, S_IRWXU) != 0) {
        if (errno != EEXIST)
            return -1; 
    }   

    return 0;
}

static int send_xmessage(Window win, Atom atom, long x)
{
  XClientMessageEvent ev;

  ev.type = ClientMessage;
  ev.window = win;
  ev.message_type = atom;
  ev.format = 32;
  ev.data.l[0] = x;
  ev.data.l[1] = CurrentTime;

  return XSendEvent(dpy, win, False, NoEventMask, (XEvent *)&ev);
}

void fork_exec(char *cmd)
{
  char *envshell, *envshellname;
  pid_t pid = fork();

  switch (pid) {
  case 0:
    setsid();
    envshell = getenv("SHELL");
    if (envshell == NULL) {
      envshell = "/bin/sh";
    }
    envshellname = strrchr(envshell, '/');
    if (envshellname == NULL) {
      envshellname = envshell;
    } else {
      envshellname++;
    }
    execlp(envshell, envshellname, "-c", cmd, NULL);
    exit(1);
    break;
  case -1:
    break;
  }
}

void send_wm_delete(Window window)
{
  int count;
  int has_delete_atom = 0;
  Atom *protocols;

  if (XGetWMProtocols(dpy, window, &protocols, &count)) {
    for (int i = 0; i < count; i++) {
      if (protocols[i] == atom_wm[AtomWMDeleteWindow]) {
        has_delete_atom = 1;
        break;
      }
    }
    XFree(protocols);
  }

  if (has_delete_atom) {
    fprintf(stderr, "Killing window kindly %#lx\n", window);
	  
    send_xmessage(window, atom_wm[AtomWMProtocols], atom_wm[AtomWMDeleteWindow]);
  } else {
    fprintf(stderr, "Force killing window %#lx\n", window);
    XKillClient(dpy, window);
  }
}

XColor create_color(char *hex)
{
  XColor color;
  Colormap colormap = DefaultColormap(dpy, 0);
  XParseColor(dpy, colormap, hex, &color);
  XAllocColor(dpy, colormap, &color);
  return color;
}

Client* find_client_by_type(Window win, int type) 
{
  YNode* curr = ylist_head(&clients);
  Client* client = NULL;
  while (curr != NULL) {
    client = (Client *)curr->data;		
		
    if (type == FRAME && client->frame == win) {
      return client;
    } else if (type == CLIENT && client->client == win) {
      return client;
    } else if (type == CLOSE_BTN && client->close_button == win) {
      return client;
    } else if (type == SHADE_BTN && client->shade_button == win) {
      return client;
    }
		
    curr = curr->next;
  }
  return NULL;
}

Client* find_client(Window win) 
{
  YNode* curr = ylist_head(&clients);
  Client* client = NULL;
  while (curr != NULL) {
    client = (Client *)curr->data;		
		
    if (client->frame == win || client->client == win || client->close_button == win || client->shade_button == win) {
      return client;
    }
		
    curr = curr->next;
  }
  return NULL;
}

void remove_client(Client* client) {
#ifdef DEBUG
  fprintf(stderr, "remove_client\n");
#endif
  XGrabServer(dpy);
	
  if (client->client) {
    XRemoveFromSaveSet(dpy, client->client);
  }
	
  XDestroyWindow(dpy, client->shade_button);
  XDestroyWindow(dpy, client->close_button);
  XDestroyWindow(dpy, client->frame);
	
  if (client->title != NULL) {
    XFree(client->title);
  }
	
  YNode* curr = ylist_head(&clients);
  while (curr != NULL) {
    Client *tmp = (Client *)curr->data;		
		
    if (tmp == client) {
      break;
    }
		
    curr = curr->next;
  }
	
  if (ylist_remove(&clients, curr, (void **)&client) != 0) {
    fprintf(stderr, "Could not remove client from list\n");
  }
	
  XSync(dpy, False);
  XUngrabServer(dpy);
}

int resistance_threshold = 50;
int snap_window_right(int x)
{
  int x_distance = x + start_window_geom.width + SNAP_BUFFER;
  return x_distance >= screen_w && x_distance <= screen_w + resistance_threshold;	
}

int snap_window_left(int x)
{
  int x_distance = x - SNAP_BUFFER;
  return x_distance <= 0 && x_distance >= -resistance_threshold;
}

int snap_window_top(int y)
{
  int y_distance = y - SNAP_BUFFER;
  return y_distance <= 0 && y_distance >= -resistance_threshold;
}

int snap_window_bottom(int y)
{
  int y_distance = y + start_window_geom.height + SNAP_BUFFER;
  return y_distance >= screen_h && y_distance <= screen_h + resistance_threshold;
}

int is_title_bar(Point p) 
{
	return p.x >= 19 && p.x <= start_window_geom.width - 7 && p.y > 0 && p.y <= FRAME_TITLEBAR_HEIGHT;
}

int is_left_frame(int x) 
{
  return x >= 0 && x <= FRAME_BORDER_WIDTH;
}

int is_right_frame(int x)
{
  return x >= (start_window_geom.width - FRAME_BORDER_WIDTH) && x <= start_window_geom.width;
}

int is_bottom_frame(int y)
{
  return y >= (start_window_geom.height - FRAME_BORDER_WIDTH) && y <= start_window_geom.height;
}

int is_lower_left_corner(Point p)
{
  return (is_bottom_frame(p.y) && p.x <= FRAME_CORNER_OFFSET) || (is_left_frame(p.x) && p.y >= (start_window_geom.height - FRAME_CORNER_OFFSET));
}

int is_lower_right_corner(Point p)
{
  return (is_bottom_frame(p.y) && p.x >= start_window_geom.width - FRAME_CORNER_OFFSET) 
    || (is_right_frame(p.x) && p.y >= start_window_geom.height - FRAME_CORNER_OFFSET);
}

int is_resize_frame(Point p)
{
  return is_bottom_frame(p.y) || is_left_frame(p.x) || is_right_frame(p.x);
}

void print_client(Client* c)
{
  fprintf(stderr, "Client {\n\ttitle = %s,\n\tclient = %#lx,\n\tframe = %#lx,\n\tclose_button = %#lx,\n\tgeometry = (%d, %d) %dx%d\n}\n", c->title, c->client, c->frame, c->close_button, c->x, c->y, c->width, c->height);	
}

int handle_xerror(Display *dpy, XErrorEvent *e)
{
  Client *c = find_client(e->resourceid);

  if (e->error_code == BadAccess && e->resourceid == root) {
    fprintf(stderr, "Root window unavailable (maybe another wm is running?)\n");
    exit(EXIT_FAILURE);
  } else {
    char msg[255];
    XGetErrorText(dpy, e->error_code, msg, sizeof(msg));
    fprintf(stderr, "X error (%#lx): %s\n", e->resourceid, msg);
  }

  if (c != NULL) {
    remove_client(c);
  }
  return 0;
}

void print_event(XEvent ev) 
{
/*  
  fprintf(stderr, "Received event: %d ", ev.type);
  switch(ev.type) {
    case KeyPress: fprintf(stderr, "[KeyPress]\n"); break;
    case KeyRelease: fprintf(stderr, "[KeyRelease]\n"); break;
    case ButtonPress: fprintf(stderr, "[ButtonPress]\n"); break;
    case ButtonRelease: fprintf(stderr, "[ButtonRelease]\n"); break;
    case MotionNotify: fprintf(stderr, "[MotionNotify]\n"); break;
    case Expose: fprintf(stderr, "[Expose]\n"); break;
    case CreateNotify: fprintf(stderr, "[CreateNotify]\n"); break;
    case DestroyNotify: fprintf(stderr, "[DestroyNotify]\n"); break;
    case ReparentNotify: fprintf(stderr, "[ReparentNotify]\n"); break;
    case ConfigureRequest: fprintf(stderr, "[ConfigureRequest]\n"); break;
    case ConfigureNotify: fprintf(stderr, "[ConfigureNotify]\n"); break;
    case MapRequest: fprintf(stderr, "[MapRequest]\n"); break;
    case MapNotify: fprintf(stderr, "[MapNotify]\n"); break;
    case UnmapNotify: fprintf(stderr, "[UnmapNotify]\n"); break;
    case EnterNotify: fprintf(stderr, "[EnterNotify]\n"); break;
    default: fprintf(stderr, "[Unknown event]\n"); break;
  }
*/
}
