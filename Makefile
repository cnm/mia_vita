.PHONY: interruption sender receiver init_counter

all: interruption sender receiver init_counter

interruption:
	make -C interruption

sender:
	make -C kernel_sender

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

clean: interruption_clean sender_clean receiver_clean
