#!/bin/bash

LED_GPS_START_LOOKUP="0x0500000000"             # First green and first yellow on
LED_GPS_NOT_FOUND_BLINK="0x0900000000"          # First green and second yellow on
LED_GPS_FOUND="0x0300000000"                    # Two greens on

/usr/local/bin/ts7500ctl --setdio=${LED_GPS_START_LOOKUP}
COUNTER=0
NO_GPS=1
while [ $COUNTER -lt 50 -a $NO_GPS -ne 0 ]; do
    /root/init_counter -t 20 2>&1 | tee -a /root/logGps >> /tmp/data/logGps

    NO_GPS=${PIPESTATUS[0]}                                              # This is equal to $? (return code) of the first pipe
    let COUNTER=COUNTER+1

    if [ $NO_GPS -ne 0 ]
    then                                                                 # Error, let's blink for two seconds
        /usr/local/bin/ts7500ctl --setdio=${LED_GPS_NOT_FOUND_BLINK}
        /usr/local/bin/ts7500ctl --setdiodir=0x1f00000000
        sleep 2
        /usr/local/bin/ts7500ctl --setdio=${LED_GPS_START_LOOKUP}
        /usr/local/bin/ts7500ctl --setdiodir=0x1f00000000
    else                                                                 # GPS acquire time and lock
        /usr/local/bin/ts7500ctl --setdio=${LED_GPS_FOUND}
        /usr/local/bin/ts7500ctl --setdiodir=0x1f00000000
    fi
done

killall -9 xuartctl
