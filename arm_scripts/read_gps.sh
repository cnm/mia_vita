#!/bin/bash

COUNTER=0
NO_GPS=1
while [ $COUNTER -lt 50 -a $NO_GPS -ne 0 ]; do
    /root/init_counter -t 20 2>&1 | tee -a /root/logGps >> /tmp/data/logGps            # This redirects stderr and stdout

    NO_GPS=$?
    let COUNTER=COUNTER+1

    if [ $NO_GPS -ne 0 ]
    then                                                  # Error
        /usr/local/bin/ts7500ctl --setdio=0x0D00000000    # FIRST, THIRD AND FOURTH
        /usr/local/bin/ts7500ctl --setdiodir=0x1f00000000
        sleep 2
        /usr/local/bin/ts7500ctl --setdio=0x0500000000    # FIRST AND THIRD
        /usr/local/bin/ts7500ctl --setdiodir=0x1f00000000
    else                                                  # Ok
        /usr/local/bin/ts7500ctl --setdio=0x0300000000    # FIRST and SECOND
        /usr/local/bin/ts7500ctl --setdiodir=0x1f00000000
    fi
done

killall -9 xuartctl
