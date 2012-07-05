#!/bin/bash

NODE=`hostname | tr -d "mv"`

case "$1" in
 start)
  echo -n "Bringing interface rausbwifi up..."

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
  ifconfig ra0 192.168.7.$NODE
  if [ $? -ne 0 ]
  then
	echo "FAILED"
	exit 1
  fi

  echo "done"
  ;;
 stop)
  echo -n "Bringing rausbwifi down..."
  ifconfig ra0 down
  if [ $? -ne 0 ]
  then
	echo "FAILED"
	exit 1
  fi
  echo "done"
  ;;
 *)
  echo "Usage: wifi.sh [start|stop]"
  exit 1
esac
