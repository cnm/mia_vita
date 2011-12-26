CC=gcc
CFLAGS=-g
#CFLAGS += -D__GPS__
EXE=main

all: syscall_wrapper.o list.o miavita_packet.h main.c
	$(CC) $(CFLAGS) -o $(EXE) main.c syscall_wrapper.o list.o

list.o: list.h list.c
	$(CC) $(CFLAGS) -c list.c

syscall_wrapper.o: syscall_wrapper.h syscall_wrapper.c
	$(CC) $(CFLAGS) -c syscall_wrapper.c

clean:
	rm -f *~ *.o $(EXE)

very_clean: clean
	rm -f *.json *.bin