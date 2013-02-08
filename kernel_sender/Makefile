obj-m += sender_kthread.o

# KERNEL_PATH=/lib/modules/$(shell uname -r)/build
KERNEL_PATH= ../ts7500_kernel/

EXTRA_CFLAGS += -I$(PWD)/../interruption/

# EXTRA_CFLAGS += -D__DEBUG__

all:
	@echo "\n\n\t\t ##### Compiling interruption Module \t####"
	make -C ../interruption/
	@echo "\n\n\t\t ##### Copying Symvers file \t\t####"
	cp -v ../interruption/Module.symvers .
	@echo "\n\n\t\t ##### Finally making kernel sender \t####"
	make -C $(KERNEL_PATH) M=$(PWD) modules

clean:
	make -C $(KERNEL_PATH) M=$(PWD) clean
	rm -f *~ Module.symvers
