DISTANCES="50 90"
MIDDLE="NO YES"
ROUTING="STATIC BATMAN"
SAMPLE_PER_SECOND="25 50"

SEED=`seq 1 30`

############# TIMERS #########
T_INITIAL_HUMAN=10
EXEC=${1}

## Wait initial time (for human synchronization)
sleep ${T_INITIAL_HUMAN}

for dist in ${DISTANCES};
do
    for mid in ${MIDDLE};
    do
        for route in ${ROUTING};
        do
            for sps in ${SAMPLE_PER_SECOND};
            do
                for seed in ${SEED};
                do

                    TEST_NAME="distance_${dist}_middle_${mid}_routing_${route}"
                    TEST_SEED=${seed}
                    SPS=${sps}

                    sh server_script.sh ${EXEC} ${TEST_NAME} ${TEST_SEED} ${SPS}
                done #SEED
            done #SAMPLE_PER_SECOND

        done #ROUTING

        #warn user
    done #MIDDLE

    #warn_user
done #DISTANCE

