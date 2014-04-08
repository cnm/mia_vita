# CC=gcc
CC=../crosstool/arm-unknown-linux-gnu/bin/arm-unknown-linux-gnu-gcc
CFLAGS=-I .
DEPS = fake_sender.h
OBJ = fake_sender.o

fake_sender: fake_sender.c
		$(CC) -o $@ $^ $(CFLAGS)

rebuild: clean fake_sender

clean:
		rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ fake_sender
