obj-m := int_mod.o
int_mod-objs := int.o fpga.o proc_entry.o
KDIR := /lib/modules/2.6.24.4/build/ 
PWD := $(shell pwd)

# EXTRA_CFLAGS += -D__GPS__

all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

print:
	echo $(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
	rm -f modules.order

rebuild: clean all
