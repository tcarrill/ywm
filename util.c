#include "util.h"

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
  printf("remove_client\n");
  XGrabServer(dpy);
	
  if (client->client) {
    XRemoveFromSaveSet(dpy, client->client);
  }
	
  XDestroyWindow(dpy, client->close_button);
  XDestroyWindow(dpy, client->shade_button);	
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

int snap_buffer = 25;
int resistance_threshold = 50;
int snap_window_right(int x)
{
  int x_distance = x + window_start.width + snap_buffer;
  return x_distance >= screen_w && x_distance <= screen_w + resistance_threshold;	
}

int snap_window_left(int x)
{
  int x_distance = x - snap_buffer;
  return x_distance <= 0 && x_distance >= -resistance_threshold;
}

int snap_window_top(int y)
{
  int y_distance = y - snap_buffer;
  return y_distance <= 0 && y_distance >= -resistance_threshold;
}

int snap_window_bottom(int y)
{
  int y_distance = y + window_start.height + snap_buffer;
  return y_distance >= screen_h && y_distance <= screen_h + resistance_threshold;
}

void print_client(Client* c)
{
  printf("Client {\n\tclient = %#lx,\n\tframe = %#lx,\n\tclose_button = %#lx,\n\tshade_button = %#lx\n}\n", c->client, c->frame, c->close_button, c->shade_button);	
  fflush(stdout);
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
