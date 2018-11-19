CC = gcc
LIBS = -L/usr/X11/lib -L/opt/X11/lib/pkgconfig/ -lX11 -lXext
INCLUDES = -I/opt/X11/include/freetype2/ -arch x86_64 -I/opt/X11/include/
CFLAGS = -Wall
OBJS = ywm.o menu.o event.o util.o ylist.o
BIN = ywm

all: $(BIN)
	
%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS) $(INCLUDES)

$(BIN): $(OBJS)
	$(CC) $(LIBS) -o $@ $(OBJS) $(INCLUDES)
		
clean:
	rm -f $(BIN )$(OBJS)
	
install:
	mkdir -p ~/.ywm && cp -i ywm.menu ~/.ywm/ywm.menu && chmod 644 ~/.ywm/ywm.menu