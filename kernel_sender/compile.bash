#!/bin/bash

make KERNEL_PATH=../ts7500_kernel/ clean
cd ../interruption; make clean; make; cp Module.symvers ../kernel_sender; cd -
make KERNEL_PATH=../ts7500_kernel/
