CC = gcc
CFLAGS = -Wall -I.
OBJ = ywm.o util.o menu.o

%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

ywm: ywm.o util.o menu.o
	gcc -lX11 -o ywm ywm.o util.o menu.o -I.

clean:
	rm -f *.o
