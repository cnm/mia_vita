obj-m += sender_kthread.o

# KERNEL_PATH=/lib/modules/$(shell uname -r)/build
KERNEL_PATH= ../ts7500_kernel/

EXTRA_CFLAGS += -I$(PWD)/../interruption/ 
#EXTRA_CFLAGS += -DDBG 
#EXTRA_CFLAGS += -D__GPS__

all:	
	make -C $(KERNEL_PATH) M=$(PWD) modules

clean:
	make -C $(KERNEL_PATH) M=$(PWD) clean
	rm -f *~
