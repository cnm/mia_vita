SINK_NODE_IP="192.168.0.3"
CLIENT_NODE_IP="192.168.0.1"

N_PACKETS=10

RESULTS_FILE=result.txt

############# Input ##########
TEST_NAME=${1}
TEST_VARIABLE_NUMBER=${2}
SAMPLES_PER_SECOND=${3}

############# TIMERS #########
T_SENDING_ALL_PACKETS=$(echo "${N_PACKETS} * ${SAMPLES_PER_SECOND}" | bc)
T_POSSIBLE_SENDING_DELAYS=1
T_WAIT_LOST_PACKETS=1

#===================================
# Do the preparations

## Get the current time
date=`date +"%d_%m_%y_%s"`

## Save the kernel version TODO
kernel_version=`uname -a`

## Create the global folder
mkdir -p results/${TEST_NAME}
#===================================

#===================================
# Server collettor

## Get current time
cur_time=`date +"%s"`

## Start server
#./sampler ${SINK_NODE_IP}:${PORT_NUMBER} -sink
touch ${RESULTS_FILE}

## Sleep x time
sleep `echo ${T_SENDING_ALL_PACKETS} + ${T_POSSIBLE_SENDING_DELAYS}| bc`

## Kill the process
pkill -9 sampler

## Move the files
mv -v ${RESULTS_FILE} ./results/${date}/${TEST_NAME}/result_${TEST_VARIABLE_NUMBER}_${cur_time}.txt

## Wait server time after
sleep ${T_WAIT_LOST_PACKETS}
#===================================
