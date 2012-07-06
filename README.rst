README
------

File Structure
==============

This is the file structure::

    .
    clean               - Directory for cleaning up the TS-7500 debian partition
    crosstool           - Crosstools binaries to compile for ARM
    documentation       - Documentation
    fred_framework      - 
    interruption        - Module to treat the two interruptions
    kernel_sender       - Fred module which sends packets
    old_stuff           - Just for historical purposes. Should be deleted soon.
    rt2501              - Module changed to do the synchronization
    rt3070              - Module rt3070 for the wireless devices. It has not be changed.
    syscall             - Simple syscall to access the time set by the GPS
    timestamp-module    - 
    ts7500ctl           - Binary of the ts7500ctl not changed. Just usefull to test big banging.
    ts7500_kernel       - Modified kernel for working in the TS7500


How to pull the git sub modules
===============================

* Pull the main module:
$> git clone "git@github.com:cnm/mia_vita.git"

* Optional check the status
$> git submodule status

* First add the submodule repository URLs to .git/config by running:
$> git submodule init

* Update the submodules
$> git submodules update

Development Username and password
=================================

*Arm devices*:

* Username: root
* Password: cnm

*Laptop*:

* Username: miavita
* password: inesc-id

FIRST STEPS - Connect to the RT7750 with serial cable
=====================================================

* Install minicom
* Run sudo minicom -s
* Set in Serial Port Setup

  * Serial Device: /dev/ttyUSB0 (or /dev/ttyUSB1)

  * Bps: 115200 8N1

  * Hardware Flow Control: No


Documentation URLs
==================
In here put the cites with documentation for the little arms:

* http://tech.groups.yahoo.com/group/ts-7000/

Sites to discover what chipset is in what wireless card

* http://wireless.kernel.org/en/users/Drivers/
* http://linux-wless.passys.nl/

Filesystem and kernels
======================
http://tagus.inesc-id.pt/~jtrindade/mia_vita/

Stable image is at:

http://tagus.inesc-id.pt/~jtrindade/mia_vita/block_images/mia_vita_image_stable


Establishing AD-HOC Communication
=================================

PC1 - LGs laptops from INESC
PC2 - TS-7550s

Run in PC1
##########
* ifconfig eth1 down
* ifconfig eth1 up
* iwconfig eth1 essid teste mode Ad-Hoc channel 1 ap 02:0c:f1:b5:8f:01 key off
* ifconfig eth1 192.168.0.1

Run in PC2
#########
* Edit the file /etc/modprobe.d/blacklist and add rt73usb
* Copy the file "rt73_ts7500.ko" to "/lib/modules/2.6.24.4/kernel/drivers/net/wireless/rt2x00". You may find the rt_ts7500.ko file in this zip file: "ftp://ftp.embeddedarm.com/ts-arm-sbc/ts-7500-linux/binaries/wifi-g-usb-2_rt2501usb-binaries.tar.gz"
* Run depmod -a
* Restart (don't really now if it is necessary)
* ifconfig rausbwifi up
* iwconfig rausbwifi essid teste mode Ad-Hoc channel 1 ap 02:0C:F1:B5:CC:5D key off
* ifconfig rausbwifi 192.168.0.25

Autostart interfaces in the TS7550
##################################
I added the following lines to /etc/network/interfaces:

auto rausbwifi
iface rausbwifi inet static
    address 192.168.0.25
    netmask 255.255.255.0
    pre-up /root/adhoc.sh

Note that the /root/adhoc.sh is a script with the previous instructions (don't forget to markit executable)

Placa Wireless IOGEAR (Can't make it work in adhoc)
====================================================

*Modulos:*
zd1211 -> Old module developed by the company (available in sourceforge)
zd1211rw -> New module developed by the community (available since 2.6.18-rc1)

*Referencias:*
Site of the module:
http://wiki.debian.org/zd1211rw

Site to compile the module + arguments of the iwconfig:
https://docs.google.com/viewer?url=http://www.linuxowl.com/ffs/DocsSoftware/SWMULZ-5400-Linux-UserGuide.pdf

Automatically boot into Debian linux
====================================
In linux initrd (busybox) in the root directory issue:

rm linuxrc; ln -sf linuxrc-sdroot linuxrc; save

Boot from sdcard via hardware
=============================

Connect pin 7 to GND

Changes done in the Operating System
====================================

Fix the name of the wireless card
---------------------------------
Add the file "01-our-rewrite.rules" to /etc/udev.d/rules with the following text:

   # All ralink wireless are named rausbwifi
   SUBSYSTEM=="net", ACTION=="add", KERNEL=="ra*", NAME="rausbwifi"

Blacklist the rt73 usb driver
-----------------------------
add "blacklist rt73usb" to /etc/modprobe.d/blacklist

Startup adhoc at the beggining
-------------------------------
In /etc/network/interfaces put:

    auto rausbwifi
    iface rausbwifi inet static
    address 192.168.0.3
    netmask 255.255.255.0
    pre-up /root/adhoc.sh rausbwifi 192.168.1.3

and /root/adhoc.sh should contain:

#!/bin/bash

    ifconfig $1 up
    iwconfig $1 mode managed
    sleep 3
    ifconfig $1 down
    ifconfig $1 up
    iwconfig $1 mode ad-hoc essid teste channel 1 ap 02:0C:F1:B5:CC:5D
    iwconfig $1 rate 1M
    ifconfig $1 $2

How to compile a user program in another machine for ts-7500
============================================================
* First setup the cross-compile. Download it from: ftp://ftp.embeddedarm.com/ts-arm-sbc/ts-7500-linux/cross-toolchains/crosstool-linux-gcc-4.2.1-glibc-2.7-oabi.tar.gz

* Untar it: tar xvzf crosstool-linux-gcc-4.2.1-glibc-2.7-oabi.tar.gz

* cd into usr/local/arm-oabi-toolchain/arm-unknown-linux-gnu/bin/ 

* Check if you have a broken link. In my case ``ls -l`` gives me this broken link: arm-unknown-linux-gnu-cc -> /usr/local/arm-oabi-toolchain/arm-unknown-linux-gnu/bin/arm-unknown-linux-gnu-gcc

* Fix it: 

    $ rm arm-unknown-linux-gnu-cc

    $ ln -s \`pwd\`/arm-unknown-linux-gnu-gcc arm-unknown-linux-gnu-cc

* Now to compile programs for the ts-7500 just use ``arm-unknown-linux-gnu-gcc`` instead of ``gcc``

How to compile kernel for the TS-7500 node
==========================================

* Download the kernel from: ftp://ftp.embeddedarm.com/ts-arm-sbc/ts-7500-linux/sources/linux-2.6.24-ts-src-aug102009.tar.gz
 (or fetch it from: http://github.com/joninvski/ts_7500_kernel )

* Download the crosstool chain: ftp://ftp.embeddedarm.com/ts-arm-sbc/ts-7500-linux/cross-toolchains/crosstool-linux-gcc-4.2.1-glibc-2.7-oabi.tar.gz
 (or fetch it from: http://github.com/joninvski/arm-uclibc-3.4.6 )

* Download the module for the wireless card: ftp://ftp.embeddedarm.com/ts-arm-sbc/ts-7500-linux/sources/wifi-g-usb-2_rt2501usb-sources.tar.gz
 (or fetch it from: http://github.com/joninvski/USB_Wifi_RT2501_TS-7500 )

First compile the kernel
------------------------

* In the 2.6.24.4-cavium directory change the Makefile pointing it to the correct path. In my case:

  * CROSS_COMPILE   ?= /home/workspace/plaquinhas/kernel/arm-uclibc-3.4.6/bin/arm-linux-

* Put the crosstoll chain in the path

* Run: $> make ts7500_defconfig

* Run: $> make menuconfig
(If there is any error compiling menuconfig just install the package libncurses-dev)

* Go to networking and select all the modules necessary for iptables/netfilter
(The .config present in the git repository contains this information)

* Run: $> make modules; make modules_install
(in here i did a litlle trick: chmod a+w /lib/modules to be able to install modules whitout being root)

Copy the kernel to the sd card
------------------------------

* Put the sdcard in the computer (let's assume sdb)

* Run: dd if=arch/arm/boot/zImage of=/dev/sdb2\

* Mount /dev/sdb4

* Copy the modules present in /lib/modules/2.6.24.4/ to the card 4th partition (to the same directory)

Compile the usb wifi card driver
--------------------------------

* Go the the directory of the usb wifi source code.

* In the Makefile change the cross tools path and the target to 7500
(you can find these changes in the git repository)

* make

* Copy the ts73.ko file to the /lib/modules/2.6.24.4/kernel/drivers/net/wireless/rt2x00/rt73_ts7500.ko (note this is in the forth partition of the sd-card)o

* You should probably (not tested) run depmod on the arm node (then restart)


Run the kernel from the sd-card
-------------------------------

* Put the jumpers in the development board: JP1 = ON; JP2 = OFF

* Do a depmod -a to do all module dependencies

Copy the kernel and initrd to the flash in the arm
--------------------------------------------------

* On my pc (I cannot to this in the card) I copy the sdb2 and sdb3 partitions to two files and then use those files to copy to the flash. This is how to do it.

* Put the sd-card on the pc

* dd if=/dev/nbd2 of=/tmp/zImage
* dd if=/dev/nbd3 of=/tmp/initrd
* Copy both these files to the /dev/ndb4 file system (mount it!!!!!!)
* Unmount /dev/ndb4 after copy
* Put the sd-card on the arm and then turn it up
* Do: 
 * spiflashctl -W 4095 -z 512 -k part1 -i /temp/zImage
 * spiflashctl -W 32 -z 65536 -k part2 -i /temp/initrd
 * sync


Add a batman service at startup
==============================
* update-rc.d batman-adv defaults

Udev rule
=========

On some linux distros users experience some minicom problems, while using the serial cable to connect to the development board. In other words, minicom stops working wtih an error message similar to::

   Unable to open /dev/ttyUSB0

This happens because the kernel keeps changing the device name and ttyUSB0 is now ttyUSB1. This can be avoided by creating an udev rule. The following udev rule is the simplest one, which provides a workaround to this problem:

* Open file /etc/udev/rules.d/99_serial_cable.rules
* Write: SUBSYSTEM=="usb", KERNEL=="ttyUSB*", NAME="ttyUSB0"

Or you can just copy and execute the command::

   sudo sh -c 'echo "SUBSYSTEM==\"usb\", KERNEL==\"ttyUSB*\", NAME=\"ttyUSB0\"" > /etc/udev/rules.d/99_serial_cable.rules'

You will need to restar udev or your pc.

Keep in mind that this rule is very simple and it only tells the udev layer to give the name ttyUSB0 to every device that the kernel reports as beginning with ttyUSB. If you ever need to connect two serial cables via USB adapters, you'll need to add another parameter to the rule above. To do this we first need the device ID for each usb adapter. Issue::

   lsusb

Which should give you an output similar to::


   Bus 005 Device 001: ID 1d6b:0001 Linux Foundation 1.1 root hub
   Bus 004 Device 002: ID 0b05:1712 ASUSTek Computer, Inc. BT-183 Bluetooth 2.0+EDR adapter
   Bus 004 Device 001: ID 1d6b:0001 Linux Foundation 1.1 root hub
   Bus 003 Device 002: ID 04f3:0210 Elan Microelectronics Corp. AM-400 Hama Optical Mouse

Now, what we're looking for is the device id, which is the second hexadecimal number in the ID field. For example, the blue tooth adapter has a device ID of 1712.

The udev rule for multiple adapters becomes::

   SUBSYSTEM=="usb", ATTR{idProduct}=="0001", NAME="ttyUSB0"
   SUBSYSTEM=="usb", ATTR{idProduct}=="0002", NAME="ttyUSB1"

The udev will give the name ttyUSB0 to the usb adapter with id 0001 and ttyUSB1 to the usb adapter with id 0002.


Compile spictl
==============

Erase the LD_FLAGS variable from the Makefile


Update the FPGA
===============

Get the new FPGA from::

    wget ftp://ftp.embeddedarm.com/ts-arm-sbc/ts-7500-linux/binaries/ts-bitstreams/ts7500_opencore-rev5-8XUART.vme.gz

Compile the ts7500ctl tool::

    wget ftp://ftp.embeddedarm.com/ts-arm-sbc/ts-7500-linux/sources/ts7500ctl.c
    wget ftp://ftp.embeddedarm.com/ts-arm-sbc/ts-7500-linux/sources/vmopcode.h
    wget ftp://ftp.embeddedarm.com/ts-arm-sbc/ts-7500-linux/sources/ispvm.c
    gcc -Wall -O -o ts7500ctl ts7500ctl.c ispvm.c

Load the new FPGA::

    ./ts7500ctl -l ts7500_opencore-rev5-8XUART.vme.gz

Test::

    arm3:~# ./spictl -l 1 -w 0B:00:20:00:00 -r 32 | hexdump -C
    00000000  20 94 e0 d4 30 03 e0 07  60 07 e0 64 00 08 e0 07  | ...0...`..d....|
    00000010  a0 03 e0 83 74 37 e0 a0  00 10 23 00 13 40 9f e4  |....t7....#..@..|

How to add a system call on arm
===============================

Adding a system call is not as straight forward as creating a kernel module. The issue is that the system call table is a static table that resides inside
the kernel. Therefore it cannot be modified dynamically. More specifically you can modify the table but it is impossible to extend it. As a result, adding 
a system call requires recompiling the kernel itself.

First thing first, download the kernel sources for the ts7500 board. cd into the downloaded sources and lets begin.
Adding a system call can be done in four steps:

* Edit the file ``arch/arm/kernel/calls.S`` and add a ``CALL`` statement::

    CALL(sys_mycall)

  Note: The syscall will be called ``mycall``, but here it is necessary to 
  prefix it with ``sys``. Another thing to consider is that the system call
  table size must be a multiple of 4. For example if it has 352 calls you 
  need to add 4 more calls. Use ``CALL(sys_ni_syscall)`` to add dummy system 
  calls. When you're finished take note of your system call number, in our 
  case we added 4 calls and suppose that our call is the last one, its number
  will be 356.

* Edit the file ``include/asm/unistd.h`` and add a ``define`` statement to the ones already in there::

    #define __NR_mycall       (__NR_SYSCALL_BASE+356)

  Note: The define constant must be prefixed with ``__NR_`` and notice the
  number 356.

* Create your system call. You'll need to decide which folder to put your .c file. There are a lot of choices here - fd, ipc, drivers, etc. No matter where you decide to put it you'll need to change the Makefile in it to compile your file. Suppose your file is named ``mysyscall.c``, add ``mysyscall.o`` to one of the object targets in the Makefile. Make sure that target will run, that is, if that option is enabled in the kernel. The ``mysyscall.c`` file will look something like::

    #include <linux/linkage.h>
    #include <linux/kernel.h>

    asmlinkage int sys_mysyscall(void){
     printk(KERN_EMERG "MY SYSCALL\n");
     return 1;
    }

* Finally, edit the file ``include/linux/syscalls.h`` and add your call header::

    asmlinkage int sys_mysyscall(void);

Mac Addresses
=============

Ethernet
########


    MV-47 - 00:d0:69:43:3f:ce - 99

    MV-34 - 00:d0:69:43:3f:c0 - 109

    MV-58 - 00:d0:69:43:3f:b9 - 145

    MV-50 - 00:d0:69:43:3f:c7 - 138

    MV-49 - 00:d0:69:43:3f:b3 - 143

Wireless cards
##############


    MV-61 94:0c:6d:e1:85:e5

    MV-41 e0:cb:4e:a6:5a:be

    Simao 94:44:52:01:95:b3

Setting up kthread to send data
===============================

The kthread module resides inside ``kernel_sender`` folder and has several parameters which can be useful to configure it::

    # modinfo send_kthread.ko

    filename:       sender_kthread.ko
    description:    This module spawns a thread which reads the buffer exported by João ands sends samples accross the network.
    author:         Frederico Gonçalves, [frederico.lopes.goncalves@gmail.com]
    license:        GPL v2
    depends:        int_mod
    vermagic:       2.6.24.4 mod_unload ARMv4 
    parm:           bind_ip:This is the ip which the kernel thread will bind to. Default is localhost. (charp)
    parm:           sink_ip:This is the sink ip. Default is localhost. (charp)
    parm:           sport:This is the UDP port which the sender thread will bind to. Default is 57843. (ushort)
    parm:           sink_port:This is the sink UDP port. Default is 57843. (ushort)
    parm:           node_id:This is the identifier of the node running this thread. Defaults to 0. (ushort)
    parm:           read_t:The sleep time for reading the buffer. (uint)

Every parameter has its own default value, but you'll probably want to specify ``bind_ip`` and ``sink_ip`` ::

    # insmod sender_kthread.ko bind-ip="172.20.41.138" sink-ip="172.20.41.123"

Don't forget to insert the ``int_mod.c`` module first and that's it. You'll just need to run the server program as ::

    # user/main 

You can specify aditional parameters ::
    
    # user/main -h

    Usage: ./main [-i <interface>] [-p <listen_on_port>] [-b <output_binary_file>] [-j <output_json_file>]
    -i     Interface name on which the program will listen. Default is eth0
    -p     UDP port on which the program will listen. Default is 57843
    -b     Name of the binary file to where the data is going to be written. Default is miavita.bin
    -j     Name of the json file to where the data is going to be written. Default is miavita.json
    -t     Test the program against GPS time. Make sure to compile this program with -D__GPS__.


Test Results (made by Simao)
============================

Check the following url:

http://tagus.inesc-id.pt/~spedro/MIA-VITA/

Test Image (made by Simao)
============================

This image should be used for all wireless tests from now on.

check the following url:
http://tagus.inesc-id.pt/~spedro/MIA-VITA/mia_vita_image_tests


Arms Embedded suggested this Wireless card with external antenna
================================================================

HAWNU1 Hi-Gain Wireless-150N USB Network Adapter w/Range Amplifi
http://www.gohawking.com/store/product_info.php?products_id=438&osCsid=valhhlsf19rnueqs8shgdgduk1

Compile for uCLIBC
==================

Use the crosstool/arm-uclibc-3.4.6/ not the crosstool/arm-unknown-linux-gnu/ crosstool

