CC=gcc
CFLAGS=-g
#CFLAGS += -D__GPS__
EXE=main

all: syscall_wrapper.o miavita_packet.h
	$(CC) $(CFLAGS) -o $(EXE) main.c syscall_wrapper.o

syscall_wrapper.o: syscall_wrapper.h
	$(CC) $(CFLAGS) -c syscall_wrapper.c

clean:
	rm -f *~ *.o $(EXE)