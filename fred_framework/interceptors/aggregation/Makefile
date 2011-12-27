obj-m += aggregate.o
aggregate-objs := aggregation.o byte_buffer.o

EXTRA_CFLAGS += -I$(PWD)/../../ -I$(PWD)/../ -DDBG

KERNEL_PATH=/lib/modules/$(shell uname -r)/build

all:	
	make -C $(KERNEL_PATH) M=$(PWD) modules

clean:
	make -C $(KERNEL_PATH) M=$(PWD) clean
	rm -f *~