SINK_NODE_IP="192.168.0.3"
CLIENT_NODE_IP="192.168.0.1"
SINK_NODE_IP="172.20.41.125"
CLIENT_NODE_IP="172.20.41.190"
N_PACKETS=1
PORT_NUMBER="57843"

INPUT_FILE="${N_PACKETS}\n${SINK_NODE_IP}:${PORT_NUMBER}"

echo ${INPUT_FILE} > client_in.commands

#EMULATED="-e"
RESULTS_FILE="osstatistics.something" #TODO CHANGE THIS

############# Input ##########
TEST_NAME=${1}
TEST_VARIABLE_NUMBER=${2}
SAMPLES_PER_SECOND=${3}

############# TIMERS #########
T_SENDING_ALL_PACKETS=$(echo "${N_PACKETS} * ${SAMPLES_PER_SECOND} * 0" | bc) #TODO Remove the 0 when for real
T_NET_DELAY=2
T_NODE_DELAY=2
T_JITTER=5
T_CAN=5
T_REST=5

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
./sampler ${CLIENT_NODE_IP} ${SPS} ${EMULATED} < client_in.commands &

## Sleep x time
sleep ${T_SENDING_ALL_PACKETS}
sleep ${T_NODE_DELAY}
sleep ${T_NET_DELAY}

## Kill the process
pkill -9 sampler
echo "KILLED client"

# Wait T_CAN + T_REST
sleep ${T_CAN}
sleep ${T_REST}

## Move the files
mv ${RESULTS_FILE} ./results/${date}/${TEST_NAME}/result_${TEST_VARIABLE_NUMBER}_${cur_time}.txt

## Wait server time after
#===================================
