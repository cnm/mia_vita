#### BATMAN ROUTING ###################
ifconfig bat0 
if [ $? -ne 0 ]
then
    echo "BAT0 IS NOT UP"
    exit 1
fi

##################################################################
echo "PRESS ENTER TO CHANGE TO BATMAN." 
echo "WAIT FOR CONFIRMATION TO CONTINUE CLIENT AND SERVER SCRIPTS"
##################################################################

#### STATIC ROUTING #####################

echo 

ifconfig rausbwifi:client 192.168.5.2 up
ifconfig rausbwifi 192.168.6.4
echo 1 > /proc/sys/net/ipv4/ip_forward
route add -net 192.168.5.0 netmask 255.255.255.0 dev rauswifi:client
route add -net 192.168.6.0 netmask 255.255.255.0 dev rauswifi



