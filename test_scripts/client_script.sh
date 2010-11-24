PORT_NUMBER="57843"

#EMULATED="-e"
RESULTS_FILE="ostatistics.data"

. ./parameters.sh

############# Input ##########
EXEC=${1}
TEST_NAME=${2}
TEST_VARIABLE_NUMBER=${3} #Its the seed number
SAMPLES_PER_SECOND=${4}
CLIENT_NODE_IP=${5}
SINK_NODE_IP=${6}
N_PACKETS=${7}

INPUT_FILE="${N_PACKETS}
${SINK_NODE_IP}:${PORT_NUMBER}"

echo ${INPUT_FILE} > client_in.commands

############# TIMERS #########
T_SENDING_ALL_PACKETS=`expr ${N_PACKETS} / ${SAMPLES_PER_SECOND}`

#===================================
# Do the preparations

## Get the current time
date=`date +"%d_%m_%y"`

## Save the kernel version TODO
kernel_version=`uname -a`

## Create the global folder
mkdir -p results/${date}/${TEST_NAME}
#===================================

#===================================
# Client collettor

## Get current time
cur_time=`date +"%s"`

# Sleep the JITTER_TIME
sleep ${T_JITTER}

## Start client
echo "STARTING CLIENT"
${EXEC} ${CLIENT_NODE_IP} ${SPS} ${EMULATED} < client_in.commands &

## Sleep x time
sleep ${T_SENDING_ALL_PACKETS}
sleep ${T_NODE_DELAY}
sleep ${T_NET_DELAY}

## Kill the process
pkill -SIGINT sampler
echo "KILLED client"

# Wait T_CAN + T_REST
sleep ${T_CAN}
sleep ${T_REST}

## Move the files
mv ${RESULTS_FILE} ./results/${date}/${TEST_NAME}/result_${TEST_VARIABLE_NUMBER}_${cur_time}.txt
#===================================
