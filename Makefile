LDLIBS+=-lefence -lz
CFLAGS+=-g -Wall -D_FILE_OFFSET_BITS=64

all: lswad wadfs

lswad: lswad.o wad.o

lswad.o: lswad.c
	$(CC) -c $(CFLAGS) -DPRINT_CRC32 -ansi -pedantic $<

wadfs: wadfs.o wad.o
	$(CC) $(LDFLAGS) $^ $(LOADLIBES) -o $@ -lfuse $(LDLIBS)

clean:
	rm -f lswad *.o wadfs

.PHONY: clean all
