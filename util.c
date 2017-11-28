#include "util.h"


static int send_xmessage(Window w, Atom a, long x)
{
	XClientMessageEvent e;

	e.type = ClientMessage;
	e.window = w;
	e.message_type = a;
	e.format = 32;
	e.data.l[0] = x;
	e.data.l[1] = CurrentTime;

	return XSendEvent(dpy, w, False, NoEventMask, (XEvent *)&e);
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
      send_xmessage(window, atom_wm[AtomWMProtocols], atom_wm[AtomWMDeleteWindow]);
  } else {
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
