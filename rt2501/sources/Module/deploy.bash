#!/bin/bash

make clean
make
make -C user/ clean
make -C user/ CC=arm-unknown-linux-gnu-gcc
rm -fr deploy/
mkdir deploy/
cp rt73.ko user/mkfilter user/rmfilter deploy/
tar cvzf deploy.tgz deploy/
rm -r deploy/