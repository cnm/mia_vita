# CC=gcc
.phony: receiver
CC=../../crosstool/arm-unknown-linux-gnu/bin/arm-unknown-linux-gnu-gcc
LIBS=-lpthread -lm
#CFLAGS=-g
#CFLAGS += -D__DEBUG__
EXE=receiver

all: receiver init_counter

receiver: syscall_wrapper.o list.o miavita_packet.h main.c
	$(CC) $(CFLAGS) -o $@  main.c syscall_wrapper.o list.o

list.o: list.h list.c
	$(CC) $(CFLAGS) -c list.c

syscall_wrapper.o: syscall_wrapper.h syscall_wrapper.c
	$(CC) $(CFLAGS) -c syscall_wrapper.c

gps_time.o: gps_time.h gps_time.c
	$(CC) $(CFLAGS) -c gps_time.c

gps_uartctl.o: gps_uartctl.h gps_uartctl.c
	$(CC) $(CFLAGS) -c gps_uartctl.c

init_counter: syscall_wrapper.o gps_time.o gps_uartctl.o init_counter.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f *~ *.o $(EXE) init_counter

very_clean: clean
	rm -f *.json *.bin

# $@ alvo que fez executar a regra
# $? requisitos que s�o mais novos que o alvo
# $< primeiro requisito
# $^ todos os requisitos
# $* prefixo saida (a parte apanhada pelo %)

