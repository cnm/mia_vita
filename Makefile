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

kernel:
	- make -C ts7500_kernel make

install: all
	@echo "Make sure the sdcard is mounted on /tmp/mv_card"
	@echo "\n\n----------------"
	@echo "####  Before"
	- @md5sum /tmp/mv_card/root/int_mod.ko
	- @md5sum /tmp/mv_card/root/receiver
	- @md5sum /tmp/mv_card/root/sender_kthread.ko
	- @md5sum /tmp/mv_card/root/init_counter
	- @md5sum /tmp/mv_card/root/network.sh
	- @md5sum /tmp/mv_card/root/read_gps.sh
	- @md5sum /tmp/mv_card/root/batman-adv.ko
	@echo "#### After"
	- @md5sum interruption/int_mod.ko
	- @md5sum kernel_sender/receiver/receiver
	- @md5sum kernel_sender/sender_kthread.ko
	- @md5sum kernel_sender/receiver/init_counter
	- @md5sum arm_scripts/network.sh
	- @md5sum arm_scripts/read_gps.sh
	- @md5sum modules/batman-adv.ko
	@echo "-------------\n\n"
	sudo cp -vf interruption/int_mod.ko kernel_sender/receiver/receiver kernel_sender/sender_kthread.ko kernel_sender/receiver/init_counter arm_scripts/network.sh arm_scripts/read_gps.sh modules/batman-adv.ko /tmp/mv_card/root
	sudo rm -frv /tmp/mv_card/etc/init.d/network.sh /tmp/mv_card/usr/our_modules/
	sudo ln -s /root/network.sh /tmp/mv_card/etc/init.d/network.sh

clean: interruption_clean sender_clean receiver_clean
