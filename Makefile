ROOT=..

all: itor

include $(ROOT)/libdraw3/libdraw3.mk

CFLAGS=-g -I$(ROOT)/libdraw3 #-static-libasan -fsanitize=address

itor: main.o $(LIBDRAW3)
	$(CC) $(CFLAGS) -o $@ main.o $(LIBDRAW3) $(LIBDRAW3_LIBS)

clean:
	rm -rf *.o itor

