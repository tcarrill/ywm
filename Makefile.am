bin_PROGRAMS = ywm
dist_doc_DATA = README.md LICENSE
EXTRA_DIST = autogen.sh

SUBDIRS = src

ywm_SOURCES = src/ywm.c src/event.c src/menu.c src/util.c src/ylist.c src/config.c
ywm_CPPFLAGS = -I$(srcdir) $(X11_CFLAGS) $(XEXT_CFLAGS) $(XFT_CFLAGS) $(FREETYPE_CFLAGS) -DSHAPE -DXFT -pthread
ywm_LDADD = $(X11_LIBS) $(XEXT_LIBS) $(XFT_LIBS) $(FREETYPE_LIBS) -lpthread

if DEBUG
ywm_CPPFLAGS += -g -DDEBUG
endif

.PHONE: debug
debug:
	$(MAKE) all DEBUG=1
