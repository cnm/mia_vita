###### Some validations ##############

#Check if the module for batman exists
if [ ! -e "batman-adv.ko" ]
then
    echo "Missing batman module file 'batman-adv.ko'"
    exit
fi

###############################################################
###############################################################
################ PART 2 - NODE MIDDLE BATMAN ##################
###############################################################
ifconfig rausbwifi 192.168.0.2

iwconfig rausbwifi rate 1M
iwconfig rausbwifi mode managed

insmod batman-adv.ko
batctl if add rausbwifi
ifconfig bat0 up
batmand rausbwifi

echo "MY IP IS 192.168.0.1"
echo "BATMAN ROUTING!!! Client and Server nodes may now start"
echo "Don't press any key until client and server end"
read

###############################################################
###############################################################
################ PART 3 - NODE MIDLE STATIC ###################
###############################################################
ifconfig rausbwifi 192.168.5.1

pkill batmand
batctl if del rausbwifi
ifconfig bat0 down
rmmod batman-adv
sleep ${T_BATMAN_WAIT}

ifconfig rausbwifi up
iwconfig rausbwifi rate 1M
iwconfig rausbwifi mode managed
sleep 3
ifconfig rausbwifi down
ifconfig rausbwifi up
iwconfig rausbwifi mode ad-hoc essid teste channel 1 ap 02:0C:F1:B5:CC:5D
ifconfig rausbwifi 192.168.5.2
ifconfig rausbwifi:virtual 192.168.6.2

echo "MY IP IS 192.168.5.2 and 192.168.6.2"
echo "STATIC ROUTING!!! Client and Server may now start"
read
