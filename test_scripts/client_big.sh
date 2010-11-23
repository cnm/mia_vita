if [ $# -ne 2 ]
then
    echo "usage: client_big.sh exec_file distance"
    exit 1
fi

ROUTING="STATIC BATMAN"
SAMPLE_PER_SECOND="25 50"

SEED=`seq 1 30`

############# TIMERS #########
T_INITIAL_HUMAN=10
T_BATMAN_WAIT=5

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

## Wait initial time (for human synchronization)
sleep ${T_INITIAL_HUMAN}

ifconfig rausbwifi 192.168.0.1
echo "MY IP IS 192.168.0.1"
for route in ${ROUTING};
do
    if [ "$route" == "STATIC" ]
    then
        pkill batmand &
        batctl if del rausbwifi &
        ifconfig bat0 down &
        rmmod batman-adv &
        sleep ${T_BATMAN_WAIT}

        ifconfig rausbwifi up
        iwconfig rausbwifi mode managed
        sleep 3
        ifconfig rausbwifi down
        ifconfig rausbwifi up
        iwconfig rausbwifi mode ad-hoc essid teste channel 1 ap 02:0C:F1:B5:CC:5D
        ifconfig rausbwifi 192.168.0.1
    fi

    if [ "$route" == "BATMAN" ]
    then
        insmod batman-adv.ko &
        batctl if add rausbwifi &
        ifconfig bat0 up &
        batmand rausbwifi &
        sleep ${T_BATMAN_WAIT}
    fi

    for sps in ${SAMPLE_PER_SECOND};
    do
        for seed in ${SEED};
        do
            TEST_NAME="distance_${DISTANCE}_middle_no_SPS_${sps}_routing_${route}"
            TEST_SEED=${seed}
            SPS=${sps}

            sh client_script.sh ${EXEC} ${TEST_NAME} ${TEST_SEED} ${SPS} "192.168.0.1" "192.168.0.3"
        done #SEED
    done #SAMPLE_PER_SECOND

done #ROUTING

###############################################################
###############################################################
################ PART 2 - NODE MIDLE BATMAN ###################
###############################################################
echo "MY IP IS 192.168.0.1"
echo "PREPARE FOR NODE IN THE MIDDLE AND BATMAN ROUTING!!!"
read

for sps in ${SAMPLE_PER_SECOND};
do
    for seed in ${SEED};
    do

        TEST_NAME="distance_${DISTANCE}_middle_no_SPS_${sps}_routing_${route}"

        TEST_SEED=${seed}
        SPS=${sps}

        sh client_script.sh ${EXEC} ${TEST_NAME} ${TEST_SEED} ${SPS} "192.168.0.1" "192.168.0.3"
    done #SEED
done #SAMPLE_PER_SECOND


###############################################################
###############################################################
################ PART 3 - NODE MIDLE STATIC ###################
###############################################################
echo "MY IP IS 192.168.5.1"
echo "PREPARE FOR NODE IN THE MIDDLE AND STATIC ROUTING!!!"
read

pkill batmand &
batctl if del rausbwifi &
ifconfig bat0 down &
rmmod batman-adv &
sleep ${T_BATMAN_WAIT} &

ifconfig rausbwifi up
iwconfig rausbwifi mode managed
sleep 3
ifconfig rausbwifi down
ifconfig rausbwifi up
iwconfig rausbwifi mode ad-hoc essid teste channel 1 ap 02:0C:F1:B5:CC:5D
ifconfig rausbwifi 192.168.5.1

route add default gw 192.168.5.2 rausbwifi

for sps in ${SAMPLE_PER_SECOND};
do
    for seed in ${SEED};
    do

        TEST_NAME="distance_${DISTANCE}_middle_yes_SPS_${sps}_routing_STATIC"
        TEST_SEED=${seed}
        SPS=${sps}

        sh client_script.sh ${EXEC} ${TEST_NAME} ${TEST_SEED} ${SPS} "192.168.5.1" "192.168.6.3"
    done #SEED
done #SAMPLE_PER_SECOND
