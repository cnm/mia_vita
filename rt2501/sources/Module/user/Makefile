CC=gcc
CFLAGS=-g

all: proc_write.o
	$(CC) $(CFLAGS) -o mkfilter mkfilter.c proc_write.o
	$(CC) $(CFLAGS) -o rmfilter rmfilter.c proc_write.o

proc_write.o: proc_write.h proc_write.c
	$(CC) $(CFLAGS) -c proc_write.c

install: all
	cp mkfilter rmfilter /usr/bin/

clean:
	rm -f rmfilter mkfilter *.o *~ 