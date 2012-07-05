#!/bin/sh
IP_VAR_SVR=192.168.7.45
#########################################
# Terminate iperf on receiver		# 
#########################################
killRemote(){
echo "Falta as rsa para n por pass"
ssh $IP_VAR_SVR 'killall iperf'
}

#########################################
# Define iperf parameters		#
# Usage: $1 = RATE			#
#	 $2 = SIZE			#
#########################################
runIperf() {
echo "sleep 10"
#sleep 10
#sleep 90
echo "Rate "$1" Size "$2
iperf -u -c $IP_VAR_SVR -b $(($1 * 8 * $2)) -l $2 -t 300 -i 1
#sleep 210
#killRemote
}


#runIperf 25 32
#runIperf 25 64
#runIperf 50 32
runIperf 50 64
runIperf 25 128
runIperf 13 256
runIperf 7 512
runIperf 4 1024
runIperf 2 1480
