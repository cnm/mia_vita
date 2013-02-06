#!/bin/bash

COUNTER=0
NO_GPS=1
while [ $COUNTER -lt 10 -a $NO_GPS -ne 0 ]; do
    /root/init_counter -t 60 &>> /root/logGps             # This redirects stderr and stdout
    NO_GPS=$?
    let COUNTER=COUNTER+1

    if [ $NO_GPS -ne 0 ]
    then                                                  # Error
        /usr/local/bin/ts7500ctl --setdio=0x0D00000000    # Now POWER_ON and Error
        /usr/local/bin/ts7500ctl --setdiodir=0x1f00000000
        sleep 2
        /usr/local/bin/ts7500ctl --setdio=0x0500000000    # Now POWER_ON and Error
        /usr/local/bin/ts7500ctl --setdiodir=0x1f00000000
    else                                                  # Ok
        /usr/local/bin/ts7500ctl --setdio=0x0300000000    # Now POWER_ON and GPS_LOCK
        /usr/local/bin/ts7500ctl --setdiodir=0x1f00000000
    fi
done

killall xuartctl

