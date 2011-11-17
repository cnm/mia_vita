#!/bin/bash

make KERNEL_PATH=/lib/modules/2.6.24.4/build clean
cd ../interruption; make clean; make; cp Module.symvers ../kernel_sender; cd -
make KERNEL_PATH=/lib/modules/2.6.24.4/build
