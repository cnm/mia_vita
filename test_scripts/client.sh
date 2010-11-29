if [ $# -ne 2 ]
then
    echo "usage: client_big.sh exec_file distance"
    exit 1
fi

# Load the parameters
. ./parameters.sh

############# INPUT #########
EXEC=$1
DISTANCE=$2

###### Some validations ##############
sh ./validations.sh
if [ ! $? -eq 0 ]
then
    return 1
fi

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
        ifconfig bat0 down
        rmmod batman-adv
        sleep ${T_BATMAN_WAIT}

        ifconfig rausbwifi up
        iwconfig rausbwifi mode managed
        sleep 3
        ifconfig rausbwifi down
        ifconfig rausbwifi up
        iwconfig rausbwifi mode ad-hoc essid teste channel 1 ap 02:0C:F1:B5:CC:5D
        iwconfig rausbwifi rate 1M
        ifconfig rausbwifi 192.168.0.1

        echo "Please synchronize. Don't do anything else"
        ts7500ctl -c
        read
        ts7500ctl -d &
    fi

    if [ "$route" == "BATMAN" ]
    then
        insmod batman-adv.ko
        batctl if add rausbwifi
        ifconfig bat0 up
        batmand rausbwifi
        sleep ${T_BATMAN_WAIT}

        echo "Please synchronize. Don't do anything else"
        ts7500ctl -c
        read
        ts7500ctl -d &
    fi

    for sps in ${SAMPLE_PER_SECOND};
    do
        for seed in ${SEED};
        do
            TEST_NAME="distance_${DISTANCE}_middle_no_SPS_${sps}_routing_${route}"
            TEST_SEED=${seed}
            SPS=${sps}

            if [ ${TEST_SEED} == 10 ]
            then
                echo "Please synchronize. Don't do anything else"
                ts7500ctl -c
                read
                ts7500ctl -d &
            fi

            if [ ${TEST_SEED} == 20 ]
            then
                echo "Please synchronize. Don't do anything else"
                ts7500ctl -c
                read
                ts7500ctl -d &
            fi

            sh client_script.sh ${EXEC} ${TEST_NAME} ${TEST_SEED} ${SPS} "192.168.0.1" "192.168.0.3" ${N_PACKETS} &

            echo SLEEPING BIG
            sleep `expr ${N_PACKETS} / ${SPS}`
            sleep ${T_JITTER}
            sleep ${T_LIMIT_TEST_TIME}
            pkill -9 client_script.sh
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

for SPS in ${SAMPLE_PER_SECOND};
do
    for seed in ${SEED};
    do
        TEST_NAME="distance_${DISTANCE}_middle_yes_SPS_${SPS}_routing_BATMAN"
        TEST_SEED=${seed}

        if [ ${TEST_SEED} == 10 ]
        then
            echo "Please synchronize. Don't do anything else"
            ts7500ctl -c
            read
            ts7500ctl -d &
        fi

        if [ ${TEST_SEED} == 20 ]
        then
            echo "Please synchronize. Don't do anything else"
            ts7500ctl -c
            read
            ts7500ctl -d &
        fi

        sh client_script.sh ${EXEC} ${TEST_NAME} ${TEST_SEED} ${SPS} "192.168.0.1" "192.168.0.3" ${N_PACKETS} &
        echo SLEEPING BIG
        sleep `expr ${N_PACKETS} / ${SPS}`
        sleep ${T_JITTER}
        sleep ${T_LIMIT_TEST_TIME}
        pkill -9 client_script.sh
    done #SEED
done #SAMPLE_PER_SECOND


###############################################################
###############################################################
################ PART 3 - NODE MIDLE STATIC ###################
###############################################################
pkill batmand
batctl if del rausbwifi
ifconfig bat0 down
rmmod batman-adv
sleep ${T_BATMAN_WAIT}

ifconfig rausbwifi up
iwconfig rausbwifi mode managed
sleep 3
ifconfig rausbwifi down
ifconfig rausbwifi up
iwconfig rausbwifi mode ad-hoc essid teste channel 1 ap 02:0C:F1:B5:CC:5D
iwconfig rausbwifi rate 1M
ifconfig rausbwifi 192.168.5.1
route add default gw 192.168.5.2 rausbwifi

echo "Please synchronize. Don't do anything else"
ts7500ctl -c
read
ts7500ctl -d &

echo "MY IP IS 192.168.5.1"
echo "PREPARE FOR NODE IN THE MIDDLE AND STATIC ROUTING!!!"
read

for SPS in ${SAMPLE_PER_SECOND};
do
    for seed in ${SEED};
    do

        TEST_NAME="distance_${DISTANCE}_middle_yes_SPS_${SPS}_routing_STATIC"
        TEST_SEED=${seed}

        if [ ${TEST_SEED} == 10 ]
        then
            echo "Please synchronize. Don't do anything else"
            ts7500ctl -c
            read
            ts7500ctl -d &
        fi

        if [ ${TEST_SEED} == 20 ]
        then
            echo "Please synchronize. Don't do anything else"
            ts7500ctl -c
            read
            ts7500ctl -d &
        fi

        sh client_script.sh ${EXEC} ${TEST_NAME} ${TEST_SEED} ${SPS} "192.168.5.1" "192.168.6.3" ${N_PACKETS} &

        echo SLEEPING BIG
        sleep `expr ${N_PACKETS} / ${SPS}`
        sleep ${T_JITTER}
        sleep ${T_LIMIT_TEST_TIME}
        pkill -9 client_script.sh
    done #SEED
done #SAMPLE_PER_SECOND
