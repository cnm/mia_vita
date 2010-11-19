#SINK_NODE_IP="192.168.0.3"
#CLIENT_NODE_IP="192.168.0.1"
SINK_NODE_IP="172.20.41.190"
CLIENT_NODE_IP="172.20.41.125"
PORT_NUMBER="57843"

N_PACKETS=1

RESULTS_FILE="ostatistics.data"

############# Input ##########
EXEC=${1}
TEST_NAME=${2}
TEST_VARIABLE_NUMBER=${3}
SAMPLES_PER_SECOND=${4}

############# TIMERS #########
T_SENDING_ALL_PACKETS=$(echo "${N_PACKETS} / ${SAMPLES_PER_SECOND}" | bc)
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
# Server collettor

## Get current time
cur_time=`date +"%s"`

## Start server
echo "STARTING SERVER"
${EXEC} ${SINK_NODE_IP} -s &
touch ${RESULTS_FILE}

## Sleep T_JITTER + T_SENDING_ALL_PACKETS + T_NODE_DELAY + T_NET_DELAY
sleep ${T_JITTER}
sleep ${T_SENDING_ALL_PACKETS}
echo "Client should have ended"
sleep ${T_NODE_DELAY}
sleep ${T_NET_DELAY}
sleep ${T_CAN}

## Kill the process
killall -SIGINT sampler
echo "KILLED SERVER"

## Move the files
mv ${RESULTS_FILE} ./results/${date}/${TEST_NAME}/result_${TEST_VARIABLE_NUMBER}_${cur_time}.txt

## Wait server time after
sleep ${T_REST}
#===================================
