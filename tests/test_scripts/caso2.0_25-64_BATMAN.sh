#!/bin/sh
IP_VAR=192.168.0.2
BAT_KO=/root/our_modules/batman-adv.ko
IFACE=rausbwifi
BAT_IFACE=bat0

	ifconfig ${IFACE} up
	iwconfig ${IFACE} mode ad-hoc essid teste channel 1 ap 02:0C:F1:B5:CC:5D

	echo "Bringing B.A.T.M.A.N. up..."
	insmod $BAT_KO
	batctl if add $IFACE
	ifconfig $BAT_IFACE ${IP_VAR}
	echo "Done"
#Ligar batman, tenho te ter a certeza que o gajo ligou
iperf -u -c 192.168.0.3 -b 12800 -l 64 -t 300 -i 1

echo "Terminou execucao"
#(sleep 1; echo root; sleep 1; echo cnm;sleep 1;echo 'killall iperf';sleep 1) | telnet 192.168.3.1

