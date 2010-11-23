#### BATMAN ROUTING ###################
echo RUNNING WITH BATMAN

ifconfig rausbwifi 192.168.0.2
ifconfig rausbwifi:client down
insmod batman-adv.ko
batctl if add rausbwifi
ifconfig bat0 up
batmand rausbwifi

echo "BATMAN READY."
echo ""
echo #############################################################
echo "PRESS ENTER TO CHANGE TO STATIC."
echo "WAIT FOR CONFIRMATION TO CONTINUE CLIENT AND SERVER SCRIPTS"
echo #############################################################
read

pkill batmand
batctl if del rausbwifi
ifconfig bat0 down
rmmod batman-adv

#### STATIC ROUTING #####################
echo "Now my ip is 192.168.5.2 and 192.168.6.2"

ifconfig rausbwifi:client 192.168.5.2 up
ifconfig rausbwifi 192.168.6.2
echo 1 > /proc/sys/net/ipv4/ip_forward
route add -net 192.168.5.0 netmask 255.255.255.0 dev rausbwifi:client
route add -net 192.168.6.0 netmask 255.255.255.0 dev rausbwifi
