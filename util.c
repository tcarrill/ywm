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
	  printf("Killing kindly\n");
	  fflush(stdout);
      send_xmessage(window, atom_wm[AtomWMProtocols], atom_wm[AtomWMDeleteWindow]);
  } else {
	  printf("Force killing\n");
	  fflush(stdout);
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
		
		if (client->frame == win || client->client == win || client->close_button == win) {
			return client;
		}
		
		curr = curr->next;
	}
	return NULL;
}

void remove_client(Display* dpy, Client* client) {
	XGrabServer(dpy);
	printf("In remove_client()\n");
	print_client(client);
	if (client->client != 0) {
		XReparentWindow(dpy, client->client, root, 0, 0);
		XRemoveFromSaveSet(dpy, client->client);
	}
	
	if (client->close_button != 0) {
		XDestroyWindow(dpy, client->close_button);
	}
	
	if (client->frame != 0) {
		XDestroyWindow(dpy, client->frame);
	}
	
	YNode* curr = ylist_head(&clients);
	while (curr != NULL) {
		Client *tmp = (Client *)curr->data;		
		
		if (tmp == client) {
			break;
		}
		
		curr = curr->next;
	}

	if (client->title != NULL) {
		XFree(client->title);
	}
	
	if (ylist_remove(&clients, curr, (void **)&client) != 0) {
		printf("Breakage\n");
	}
	
	printf("Managed clients: %d\n", ylist_size(&clients));
	fflush(stdout);
	XUngrabServer(dpy);
}

void print_client(Client* c)
{
	printf("Client {\n\tframe = %lu,\n\tclient = %lu,\n\tclose_button = %lu\n}\n", c->frame, c->client, c->close_button);	
	fflush(stdout);
}
