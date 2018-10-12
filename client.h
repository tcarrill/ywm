#ifndef CLIENT_H
#define CLIENT_H

typedef struct {
	int x, y;
} Point;

typedef struct {
	int x, y;
	int width, height;
} Rect;

typedef struct {
	char *title;
	Window close_button;
	Window client;
	Window frame;
} Client;

#endif