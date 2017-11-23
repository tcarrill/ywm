#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include "ywm.h"

void fork_exec(char *cmd);
void send_wm_delete(Window window);
XColor create_color(char* hex);

#endif
