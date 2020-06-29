CC = cc
LIBS = -L/usr/X11/lib -L/opt/X11/lib/ -lXft -lX11 -lXext
INCLUDES = -I/usr/include/freetype2/ -I/opt/X11/include/
CFLAGS = -Wall 
OBJS = ywm.o menu.o event.o util.o ylist.o
BIN = ywm
BINDIR = /usr/local/bin
DEFINES += -DSHAPE -DXFT
DEFINES += -DDEBUG

all: $(BIN)

%.o: %.c 
	$(CC) $(DEFINES) -c -o $@ $^ $(CFLAGS) $(INCLUDES)

$(BIN): $(OBJS)
	$(CC) $(LIBS) -o $@ $(OBJS) $(INCLUDES)		

clean:
	rm -f $(BIN) $(OBJS)

install: $(BIN)
	mkdir -p $(DESTDIR)$(BINDIR)
	cp $(BIN) $(DESTDIR)$(BINDIR)
	mkdir -p ~/.ywm && cp -i ../ywm.menu ~/.ywm/ywm.menu && chmod 644 ~/.ywm/ywm.menu
