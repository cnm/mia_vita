CC= ../crosstool/arm-unknown-linux-gnu/bin/arm-unknown-linux-gnu-gcc
FLAGS=-Wall -g
LIBS=-pthread -lm

uart_gps_test: uart_gps_test.c gps_time.o gps_uartctl.o
	$(CC) -o $@ $^ ${FLAGS} ${LIBS}

%.o: %.c %.h
	$(CC) -c $< ${FLAGS}

clean:
	rm -f *.o uart_gps_test

# $@ alvo que fez executar a regra
# $? requisitos que s�o mais novos que o alvo
# $< primeiro requisito
# $^ todos os requisitos
# $* prefixo saida (a parte apanhada pelo %)

