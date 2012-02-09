obj-m := int_mod.o
int_mod-objs := int.o fpga.o proc_entry.o
KDIR := ../ts7500_kernel/
PWD := $(shell pwd)

# EXTRA_CFLAGS += -D__GPS__

all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

print:
	echo $(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
	rm -f modules.order
	rm -f reader

rebuild: clean all
