all: interruption sender receiver init_counter

interruption:
	make -C interruption

sender:
	make -C kernel_sender

receiver:
	make -C kernel_sender/receiver

init_counter:
	make -C kernel_sender/receiver init_counter

# kernel:

