README
------
Put here the stuff to work with the RT7550

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
In linux busy-box in the root directory issue:
ln -sf linuxrc-sdroot linuxrc; save

Translation table
=================

+------+-----------------+----------+
| card |        ip       | Register |
+======+=================+==========+
|  1   | 192.168.0.1     |   MV-25  |
|  2   | 192.168.0.2     |   MV-26  |
|  3   | 192.168.0.3     |   MV-27  |
+======+=================+==========+

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

