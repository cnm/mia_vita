.PHONY: interruption sender receiver init_counter kernel install install_kernel uart_gps_test

all: interruption sender receiver init_counter uart_gps_test

# Just an "alias" to prepare the git submodules
init_git_submodules:
	git submodule init
	git submodule update

# kernel module responsible to receive CPU interruptions, read ADC, and write to buffer
interruption:
	make -C interruption

# kernel module responsible to read buffer and send messages
sender:
	cd kernel_sender && make

# userland exec that receives packets and writes json file
receiver:
	make -C kernel_sender/receiver

# userland exec that reads gps time and sets the time
init_counter:
	make -C kernel_sender/receiver init_counter

uart_gps_test:
	make -C gps_time/trimble_condor uart_gps_test

# Compiles the kernel
kernel:
	make -C ts7500_kernel

# Installs the kernel's zImage into the sdcard (ps: the zImage is the kernel)
install_kernel: kernel
	@echo "Please enter the letter for your sdcard /dev/sdX ---> "; \
	read letter_sdcard; \
	echo "Going to erase partitions /dev/sd$$letter_sdcard$$(echo 2) and 3. Press control+c to cancel"; \
	read nothing; \
	dd if=ts7500_kernel/arch/arm/boot/zImage of=/dev/sd$$letter_sdcard$$(echo 2)

build_initrd_binaries:
	make -C interruption/modified_binaries/sdctl
	make -C interruption/modified_binaries/xuartctl
	@echo "Now please mount third partition of sdcard and copy the sdctl and xuartctl files"

# Copies all necessary files to the sdcard
install: all
	@echo "\n\n----------------"
	@echo "Make sure the sdcard is mounted on /tmp/mv_card"
	@echo "####  Before"
	- @md5sum /tmp/mv_card/root/int_mod.ko
	- @md5sum /tmp/mv_card/root/sender_kthread.ko
	- @md5sum /tmp/mv_card/root/receiver
	- @md5sum /tmp/mv_card/root/init_counter
	- @md5sum /tmp/mv_card/root/network.sh
	- @md5sum /tmp/mv_card/root/read_gps.sh
	- @md5sum /tmp/mv_card/root/batman-adv.ko
	- @md5sum /tmp/mv_card/root/motd
	- @md5sum /tmp/mv_card/root/uart_gps_test
	@echo "#### After"
	- @md5sum interruption/int_mod.ko
	- @md5sum kernel_sender/sender_kthread.ko
	- @md5sum kernel_sender/receiver/receiver
	- @md5sum kernel_sender/receiver/init_counter
	- @md5sum arm_scripts/network.sh
	- @md5sum arm_scripts/read_gps.sh
	- @md5sum modules/batman-adv.ko
	- @md5sum arm_scripts/motd
	- @md5sum gps_time/trimble_condor/uart_gps_test
	@echo "-------------\n\n"
	sudo cp -vf interruption/int_mod.ko kernel_sender/receiver/receiver kernel_sender/sender_kthread.ko kernel_sender/receiver/init_counter arm_scripts/network.sh arm_scripts/read_gps.sh modules/batman-adv.ko arm_scripts/motd gps_time/trimble_condor/uart_gps_test /tmp/mv_card/root
	sudo rm -frv /tmp/mv_card/etc/init.d/network.sh /tmp/mv_card/usr/our_modules/ /tmp/mv_card/etc/motd
	sudo ln -s /root/network.sh /tmp/mv_card/etc/init.d/network.sh
	sudo ln -fs /root/motd /tmp/mv_card/etc/motd

# Clean stuff (not complete TODO)
interruption_clean:
	make -C interruption clean

sender_clean:
	- make -C kernel_sender clean

receiver_clean:
	- make -C kernel_sender/receiver clean

clean: interruption_clean sender_clean receiver_clean
