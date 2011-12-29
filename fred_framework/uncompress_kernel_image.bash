#!/bin/bash

#find the byte pattern
BASE=`od -A d -t x1 /boot/vmlinuz-$(uname -r) | grep '1f 8b 08 00' --colour | awk '{print $1}'`

echo "Found base: $BASE"

#Compute byte offset
OFFSET=`echo $BASE + 12 | bc`

echo "Computed offset: $OFFSET"

#Uncompress image
dd if=/boot/vmlinuz-$(uname -r) bs=1 skip=$OFFSET | zcat > vmlinux-$(uname -r)