#!/bin/bash 

#Let's start MiaVita stuff
NO_GPS=1
while [ $NO_GPS ne 0 ]; do
    /root/init_counter &> /root/logGps
    NO_GPS=$?
    if [ $NO_GPS -ne 0 ]
    then #Error
        /usr/local/bin/ts7500ctl --setdio=0x0D00000000 #Now POWER_ON and Error
        /usr/local/bin/ts7500ctl --setdiodir=0x1f00000000
        sleep 2
        /usr/local/bin/ts7500ctl --setdio=0x0500000000 #Now POWER_ON and Error
        /usr/local/bin/ts7500ctl --setdiodir=0x1f00000000
    else #Ok
        /usr/local/bin/ts7500ctl --setdio=0x0300000000 #Now POWER_ON and GPS_LOCK
        /usr/local/bin/ts7500ctl --setdiodir=0x1f00000000
    fi
done
