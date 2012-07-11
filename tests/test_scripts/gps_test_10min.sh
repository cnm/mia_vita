#!/usr/bin/env bash

#date -set "Mon Jul  9 16:00:58 WEST 2012"

xuartctl -p 0 -o 8o1 -s 9600 -d
./uart_gps_test &> time_gps_`date +%Y_%m_%d_%H_%m_%S`.txt

sleep 900

killall uart_gps_test