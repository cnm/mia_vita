# CROSS_COMPILE  = /home/workspace/mia_vita/ts7500_kernel/arm-uclibc-3.4.6/bin/arm-linux
# CXX = $(CROSS_COMPILE)-g++
# CC = $(CROSS_COMPILE)-gcc
# AS = $(CROSS_COMPILE)-as
# AR = $(CROSS_COMPILE)-ar
# NM = $(CROSS_COMPILE)-nm
# LD = $(CROSS_COMPILE)-ld
# OBJDUMP = $(CROSS_COMPILE)-objdump
# OBJCOPY = $(CROSS_COMPILE)-objcopy
# RANLIB = $(CROSS_COMPILE)-ranlib
# STRIP = $(CROSS_COMPILE)-strip

all: reader.c
	${CC} -o reader reader.c 

clean: 
	rm -f reader
