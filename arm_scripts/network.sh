#!/bin/bash

NODE=`hostname | tr -d "mv"`

/usr/local/bin/ts7500ctl --setdio=0x0F00000000    # Now POWER_ON and Error

echo -n "Bringing interface eth0 up ..."
ifconfig eth0 192.168.0.$NODE
echo "Done"

/usr/local/bin/xuartctl -p 0 -o 8n1 -s 9600 -d &> /root/logXuart

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

mkdir /tmp/data
mount /dev/sda1 /tmp/data

#Let's start MiaVita stuff
/root/read_gps.sh
# /bin/bash -c "nohup /root/read_gps.sh &"
sleep 2

killall -9 daqctl

#Let work with the leds
# /usr/local/bin/ts7500ctl --setdio=0x0100000000 #Now POWER_ON
# /usr/local/bin/ts7500ctl --setdiodir=0x1f00000000

# /usr/local/bin/ts7500ctl --setdio=0x0F00000000    # Now POWER_ON and Error

insmod /root/int_mod.ko &> /root/logIntMod
sleep 1


insmod /root/sender_kthread.ko bind-ip="192.168.2.$NODE" sink-ip="192.168.2.$NODE" node-id="$((NODE - 40))" &> /root/logSenderMod
# insmod /root/sender_kthread.ko bind-ip="192.168.0.$NODE" sink-ip="192.168.0.1" node-id="$((NODE - 40))" &> /root/logSenderMod
# echo "Done"

/bin/bash -c "nohup /root/receiver -i bat0 -j /tmp/data/miavita.json &"
