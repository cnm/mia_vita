CC=gcc
CFLAGS=-g
#CFLAGS += -D__GPS__
EXE=main

all: syscall_wrapper.o list.o miavita_packet.h
	$(CC) $(CFLAGS) -o $(EXE) main.c syscall_wrapper.o list.o

list.o: list.h
	$(CC) $(CFLAGS) -c list.c

syscall_wrapper.o: syscall_wrapper.h
	$(CC) $(CFLAGS) -c syscall_wrapper.c

clean:
	rm -f *~ *.o $(EXE)

very_clean: clean
	rm -f *.json *.bin