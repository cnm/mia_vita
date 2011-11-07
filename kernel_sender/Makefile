obj-m += sender_kthread.o

KERNEL_PATH=/lib/modules/$(shell uname -r)/build

all:	
	make -C $(KERNEL_PATH) M=$(PWD) modules

clean:
	make -C $(KERNEL_PATH) M=$(PWD) clean
	rm -f *~