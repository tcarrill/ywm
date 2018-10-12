CC = gcc
CFLAGS = -Wall -I.
OBJ = ywm.o menu.o event.o util.o ylist.o

%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

ywm: ywm.o menu.o event.o util.o ylist.o
	gcc -L/usr/X11/lib -lX11 -o ywm ywm.o menu.o event.o util.o ylist.o -I.

clean:
	rm -f *.o
	rm ywm
