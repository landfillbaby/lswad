LDFLAGS=-lefence
CFLAGS=-g -Wall -D_FILE_OFFSET_BITS=64

all: lswad wadfs

lswad: lswad.o wad.o

lswad.o: lswad.c
	$(CC) -c $(CFLAGS) -ansi -pedantic $<

wadfs: wadfs.o wad.o
	$(CC) -lfuse $(LDFLAGS) $^ $(LOADLIBES) -o $@

clean:
	rm -f lswad *.o wadfs

.PHONY: clean all
