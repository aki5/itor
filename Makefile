
ROOT=..

TARGET_ARCH=port
ifeq ($(shell uname -m), armv6l)
	TARGET_ARCH=arm6
endif

CFLAGS=-Os -fomit-frame-pointer -I/opt/local/include -I$(ROOT)/libdraw3 -W -Wall

OFILES=\
	main.o\
	textedit.o\

all: itor-x11 itor-linuxfb

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

include $(ROOT)/libdraw3/libdraw3.mk

itor-x11: $(OFILES) $(LIBDRAW3_X11)
	$(CC) $(CFLAGS) -o $@ $(OFILES) $(LIBDRAW3_X11) $(LIBDRAW3_X11_LIBS)

itor-linuxfb: $(OFILES) $(LIBDRAW3_LINUXFB)
	$(CC) $(CFLAGS) -o $@ $(OFILES) $(LIBDRAW3_LINUXFB) $(LIBDRAW3_LINUXFB_LIBS)

clean:
	rm -rf *.o itor-x11 itor-linuxfb perf.*

nuke: clean
	make -C ../libdraw3 clean
