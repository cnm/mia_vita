if [ $# -ne 2 ]
then
    echo "usage: intermediate_big.sh exec_file distance"
    exit 1
fi

ROUTING="STATIC BATMAN"
SAMPLE_PER_SECOND="25 50"

SEED=`seq 1 30`

############# INPUT #########
EXEC=$1
DISTANCE=$2

###### Some validations ##############

#Check if the module for batman exists
if [ ! -e "batman-adv.ko" ]
then
    echo "Missing batman module file 'batman-adv.ko'"
    exit
fi

###############################################################
###############################################################
################ PART 2 - NODE MIDDLE BATMAN ###################
###############################################################
echo "MY IP IS 192.168.0.1"
echo "BATMAN ROUTING!!!"
read

ifconfig rausbwifi 192.168.0.2

insmod batman-adv.ko
batctl if add rausbwifi
ifconfig bat0 up
batmand rausbwifi

for sps in ${SAMPLE_PER_SECOND};
do
    for seed in ${SEED};
    do

        TEST_NAME="distance_${DISTANCE}_middle_no_SPS_${sps}_routing_${route}"

        TEST_SEED=${seed}
        SPS=${sps}

        sh middle_script.sh ${EXEC} ${TEST_NAME} ${TEST_SEED} ${SPS} "192.168.0.1" "192.168.0.3"
    done #SEED
done #SAMPLE_PER_SECOND


###############################################################
###############################################################
################ PART 3 - NODE MIDLE STATIC ###################
###############################################################
echo "MY IP IS 192.168.5.1"
echo "PREPARE FOR NODE IN THE MIDDLE AND STATIC ROUTING!!!"
read
ifconfig rausbwifi 192.168.5.1

pkill batmand
batctl if del rausbwifi
ifconfig bat0 down
rmmod batman-adv

ifconfig rausbwifi up
iwconfig rausbwifi mode managed
sleep 3
ifconfig rausbwifi down
ifconfig rausbwifi up
iwconfig rausbwifi mode ad-hoc essid teste channel 1 ap 02:0C:F1:B5:CC:5D

ifconfig rausbwifi 192.168.6.1

route add default gw 192.168.5.2 rausbwifi

for sps in ${SAMPLE_PER_SECOND};
do
    for seed in ${SEED};
    do

        TEST_NAME="distance_${DISTANCE}_middle_yes_routing_STATIC"
        TEST_SEED=${seed}
        SPS=${sps}

        sh middle_script.sh ${EXEC} ${TEST_NAME} ${TEST_SEED} ${SPS} "192.168.5.1" "192.168.6.3"
    done #SEED
done #SAMPLE_PER_SECOND
