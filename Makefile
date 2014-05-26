.PHONY: interruption sender receiver init_counter

all: interruption sender receiver init_counter

interruption:
	make -C interruption

sender:
	cd kernel_sender && make

receiver:
	make -C kernel_sender/receiver

init_counter:
	make -C kernel_sender/receiver init_counter

interruption_clean:
	make -C interruption clean

sender_clean:
	- make -C kernel_sender clean

receiver_clean:
	- make -C kernel_sender/receiver clean

install: all
	@echo "Make sure the sdcard is mounted on /tmp/mv_card"
	@echo "int_mod.ko md5"
	md5sum interruption/int_mod.ko
	@echo "receiver md5"
	md5sum kernel_sender/receiver/receiver
	@echo "sender_ktread.ko md5"
	md5sum kernel_sender/sender_kthread.ko
	@echo "init_counter md5"
	md5sum kernel_sender/receiver/init_counter
	cp -vf interruption/int_mod.ko kernel_sender/receiver/receiver kernel_sender/sender_kthread.ko kernel_sender/receiver/init_counter /tmp/mv_card/root

clean: interruption_clean sender_clean receiver_clean
