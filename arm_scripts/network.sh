#!/bin/bash

# Variables
NODE=`hostname | tr -d "mv"`
LED_ALL_ON_START="0x0F00000000"

# mount the external sd card
mkdir /tmp/data
mount /dev/sda1 /tmp/data

/usr/local/bin/ts7500ctl --setdio=${LED_ALL_ON_START}             # Now POWER_ON and Error

echo -n "Bringing interface eth0 up ..."
ifconfig eth0 192.168.0.$NODE
echo "Done"

/usr/local/bin/xuartctl -p 0 -o 8n1 -s 9600 -d 2>&1 | tee /root/logXuart > /tmp/data/logXuart

echo -n "Bringing interface ra0 up..."
ifconfig ra0 up
if [ $? -ne 0 ]
then
    echo "FAILED"
    exit 1
fi
iwconfig ra0 mode ad-hoc essid 'MIA-VITA' channel 11
if [ $? -ne 0 ]
then
    echo "FAILED"
    exit 1
fi
ifconfig ra0 192.168.1.$NODE
if [ $? -ne 0 ]
then
    echo "FAILED"
    exit 1
fi
echo "Done"

echo -n "Bringing B.A.T.M.A.N. up..."
insmod /root/batman-adv.ko
batctl if add ra0
ifconfig bat0 192.168.2.$NODE

echo 1 > /proc/sys/net/ipv4/ip_forward
modprobe ip_tables
modprobe ip_conntrack
modprobe iptable_filter
modprobe ipt_state
modprobe iptable_nat
modprobe ipt_MASQUERADE
iptables -t nat -A POSTROUTING -o bat0 -j MASQUERADE

/usr/local/bin/ts7500ctl --greenledoff
/usr/local/bin/ts7500ctl --redledoff

#Let's start MiaVita stuff
/root/read_gps.sh
# /bin/bash -c "nohup /root/read_gps.sh &"
sleep 2

killall -9 daqctl

insmod /root/int_mod.ko 2>&1 | tee /root/logIntMod > /tmp/data/logIntMod
sleep 1


insmod /root/sender_kthread.ko bind-ip="192.168.2.$NODE" sink-ip="192.168.2.43" node-id="$((NODE - 40))" 2>&1 | tee /root/logSenderMod > /tmp/data/logSenderMod
# insmod /root/sender_kthread.ko bind-ip="192.168.0.$NODE" sink-ip="192.168.0.1" node-id="$((NODE - 40))" &> /root/logSenderMod
# echo "Done"

/bin/bash -c "nohup /root/receiver -i bat0 -j /tmp/data/miavita.json &"
