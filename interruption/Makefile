obj-m := int_mod.o
int_mod-objs := int.o fpga.o proc_entry.o
KDIR := ../ts7500_kernel/
PWD := $(shell pwd)

CROSS-TOOL := ../crosstool/arm-unknown-linux-gnu/bin/
CC1 := $(CROSS-TOOL)arm-unknown-linux-gnu-gcc

# EXTRA_CFLAGS += -D__GPS__
# EXTRA_CFLAGS += -D__DEBUG__

all: userland_reader
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

userland_reader: userland_reader.c
	$(CC1) -g $? -o reader

print:
	echo $(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
	rm -f modules.order
	rm -f userland_reader

rebuild: clean all
