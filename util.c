#include <signal.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include "util.h"

void fork_exec(char *cmd)
{
  char *envshell, *envshellname;
  pid_t pid = fork();

  switch (pid)
    {
    case 0:
      setsid();
      envshell = getenv("SHELL");
      if (envshell == NULL)
	{
	  envshell = "/bin/sh";
	}
      envshellname = strrchr(envshell, '/');
      if (envshellname == NULL)
	{
	  envshellname = envshell;
	}
      else
	{
	  /* move to the character after the slash */
	  envshellname++;
	}
      execlp(envshell, envshellname, "-c", cmd, NULL);
      err("exec failed, cleaning up child");
      exit(1);
      break;
    case -1:
      err("can't fork");
      break;
    }
}

XColor create_color(Display *dpy, char *hex)
{
  XColor color;
  Colormap colormap = DefaultColormap(dpy, 0);
  XParseColor(dpy, colormap, hex, &color);
  XAllocColor(dpy, colormap, &color);
  return color;
}

