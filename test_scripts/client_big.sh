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

############# INPUT #########
EXEC=$1
DISTANCE=$2

## Wait initial time (for human synchronization)
sleep ${T_INITIAL_HUMAN}

ifconfig rausbwifi 192.168.0.1
echo "MY IP IS 192.168.0.1"
for route in ${ROUTING};
do
    if [ "$route" == "STATIC" ] 
    then
	pkill batmand
	batctl if del rausbwifi
	rmmod batman-adv
    fi

    if [ "$route" == "BATMAN" ] 
    then
	insmod batman-adv.ko
	batctl if add rausbwifi
	batmand rausbwifi
    fi

    for sps in ${SAMPLE_PER_SECOND};
    do
        for seed in ${SEED};
        do
	    
            TEST_NAME="distance_${DISTANCE}_middle_no_routing_${route}"
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
	
        TEST_NAME="distance_${DISTANCE}_middle_yes_routing_BATMAN"
        TEST_SEED=${seed}
        SPS=${sps}
	
        sh client_script.sh ${EXEC} ${TEST_NAME} ${TEST_SEED} ${SPS} "192.168.0.1" "192.168.0.3"
    done #SEED
done #SAMPLE_PER_SECOND


###############################################################
###############################################################
################ PART 2 - NODE MIDLE STATIC ###################
ifconfig rausbwifi 192.168.5.1
###############################################################
echo "MY IP IS 192.168.5.1"
echo "PREPARE FOR NODE IN THE MIDDLE AND STATIC ROUTING!!!"
read

pkill batmand
batctl if del rausbwifi
rmmod batman-adv	    

route add default gw 192.168.5.2 rausbwifi

for sps in ${SAMPLE_PER_SECOND};
do
    for seed in ${SEED};
    do
	
        TEST_NAME="distance_${DISTANCE}_middle_yes_routing_STATIC"
        TEST_SEED=${seed}
        SPS=${sps}
	
        sh client_script.sh ${EXEC} ${TEST_NAME} ${TEST_SEED} ${SPS} "192.168.5.1" "192.168.6.3"
    done #SEED
done #SAMPLE_PER_SECOND
