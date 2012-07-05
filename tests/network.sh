#!/bin/bash

NODE=`hostname | tr -d "mv"`

case "$1" in
 start)
  echo -n "Bringing interface eth0 up ..."
  ifconfig eth0 192.168.0.$NODE

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
  ifconfig ra0 192.168.7.$NODE
  if [ $? -ne 0 ]
  then
	echo "FAILED"
	exit 1
  fi

  echo "done"
  ;;
 stop)
    echo -n "Bringing eth0 down..."
  ifconfig eth0 down
  echo -n "Bringing ra0 down..."
  ifconfig ra0 down
  if [ $? -ne 0 ]
  then
	echo "FAILED"
	exit 1
  fi
  echo "done"
  ;;
 *)
  echo "Usage: network.sh [start|stop]"
  exit 1
esac
