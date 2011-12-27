CC=gcc

MK_OUT=mkrule
RM_OUT=rmrule

all: mkrule rmrule

mkrule: mkrule.c cmd_line_parser.o operations.o
	$(CC) -o $(MK_OUT) mkrule.c cmd_line_parser.o operations.o

rmrule: rmrule.c cmd_line_parser.o operations.o
	$(CC) -o $(RM_OUT) rmrule.c cmd_line_parser.o operations.o

cmd_line_parser.o: cmd_line_parser.h cmd_line_parser.c
	$(CC) -c cmd_line_parser.c

operations.o: operations.h operations.c
	$(CC) -c operations.c

clean:
	rm -f *~ *.o $(MK_OUT) $(RM_OUT)

install: all
	cp $(MK_OUT) $(RM_OUT) /usr/bin

uninstall:
	rm -f /usr/bin/$(MK_OUT) /usr/bin/$(RM_OUT)