
ROOT=..

TARGET_ARCH=port
ifeq ($(shell uname -m), armv6l)
	TARGET_ARCH=arm6
endif

CFLAGS=-g -I/opt/local/include -I$(ROOT)/libdraw3 -W -Wall

all: itor-x11 itor-linuxfb

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

include $(ROOT)/libdraw3/libdraw3.mk

itor-x11: main.o $(LIBDRAW3_X11)
	$(CC) $(CFLAGS) -o $@ main.o $(LIBDRAW3_X11) $(LIBDRAW3_X11_LIBS)

itor-linuxfb: main.o $(LIBDRAW3_LINUXFB)
	$(CC) $(CFLAGS) -o $@ main.o $(LIBDRAW3_LINUXFB) $(LIBDRAW3_LINUXFB_LIBS)

clean:
	rm -rf *.o itor-x11 itor-linuxfb perf.*

nuke: clean
	make -C ../libdraw3 clean
