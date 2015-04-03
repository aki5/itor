
ROOT=..

TARGET_ARCH=port
ifeq ($(shell uname -m), armv6l)
	TARGET_ARCH=arm6
endif

TARGETS=itor-x11
ifeq ($(shell uname -s), Linux)
	TARGETS+=itor-linuxfb
endif

CFLAGS=-Os -fomit-frame-pointer -I/opt/local/include -I$(ROOT)/libdraw3 -W -Wall
#CFLAGS=-g -I/opt/local/include -I$(ROOT)/libdraw3 -W -Wall

OFILES=\
	main.o\
	textedit.o\
	dragborder.o\

all: $(TARGETS) 

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

include $(ROOT)/libdraw3/libdraw3.mk

itor-x11: $(OFILES) $(LIBDRAW3_X11)
	$(CC) $(CFLAGS) -o $@ $(OFILES) $(LIBDRAW3_X11) $(LIBDRAW3_X11_LIBS)

itor-linuxfb: $(OFILES) $(LIBDRAW3_LINUXFB)
	$(CC) $(CFLAGS) -o $@ $(OFILES) $(LIBDRAW3_LINUXFB) $(LIBDRAW3_LINUXFB_LIBS)

clean:
	rm -rf *.o $(TARGETS) perf.*

nuke: clean
	make -C ../libdraw3 clean
