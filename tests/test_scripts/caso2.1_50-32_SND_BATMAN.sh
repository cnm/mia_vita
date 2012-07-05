#!/bin/sh
IP_VAR=192.168.0.1
BAT_KO=/root/our_modules/batman-adv.ko
IFACE=rausbwifi
BAT_IFACE=bat0
FILE=CASO1.1-50-32-$(uname -n)-$(date +%T-%d-%m-%G)
	ifconfig ${IFACE} up
	iwconfig ${IFACE} mode ad-hoc essid teste channel 1 ap 02:0C:F1:B5:CC:5D

	echo "Bringing B.A.T.M.A.N. up..."
	insmod $BAT_KO
	batctl if add $IFACE
	ifconfig $BAT_IFACE ${IP_VAR}
	echo "Done"
  
#tenho de encontrar uma forma de verificar se o batman esta a funcionar
iperf -u -c 192.168.0.3 -b 25600 -l 32 -t 300 -i 1
#enviar os dados de volta para o controlo

