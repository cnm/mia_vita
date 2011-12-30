obj-m += aggregate.o
aggregate-objs := aggregation.o byte_buffer.o consuela.o

EXTRA_CFLAGS += -I$(PWD)/../../ -I$(PWD)/../ -DDBG

KERNEL_PATH=/lib/modules/$(shell uname -r)/build

all:	
	echo $(EXTRA_CFLAGS)
	make -C $(KERNEL_PATH) M=$(PWD) modules

clean:
	make -C $(KERNEL_PATH) M=$(PWD) clean
	rm -f *~