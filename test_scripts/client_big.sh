DISTANCES="50 90"
MIDDLE="NO YES"
ROUTING="STATIC BATMAN"
SAMPLE_PER_SECOND="25 50"

SEED=`seq 1 30`

############# TIMERS #########
T_INITIAL_HUMAN=10

############# INPUT #########
EXEC=$1

## Wait initial time (for human synchronization)
sleep ${T_INITIAL_HUMAN}

for dist in ${DISTANCES};
do
    for mid in ${MIDDLE};
    do
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

                    TEST_NAME="distance_${dist}_middle_${mid}_routing_${route}"
                    TEST_SEED=${seed}
                    SPS=${sps}

                    sh client_script.sh ${EXEC} ${TEST_NAME} ${TEST_SEED} ${SPS}
                done #SEED
            done #SAMPLE_PER_SECOND

        done #ROUTING

        #warn user
    done #MIDDLE

    #warn_user
done #DISTANCE

