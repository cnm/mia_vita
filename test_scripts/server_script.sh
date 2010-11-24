PORT_NUMBER="57843"
echo `date +"%s"`

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
# Server collettor

## Get current time
cur_time=`date +"%s"`

## Start server
echo "STARTING SERVER"
${EXEC} ${SINK_NODE_IP} -s &

## Sleep T_JITTER + T_SENDING_ALL_PACKETS + T_NODE_DELAY + T_NET_DELAY
sleep ${T_JITTER}
sleep ${T_SENDING_ALL_PACKETS}
echo "Client should have ended"
sleep ${T_NODE_DELAY}
sleep ${T_NET_DELAY}
sleep ${T_CAN}

## Kill the process
pkill -SIGINT sampler
echo "KILLED SERVER"

# Wait T_REST
sleep ${T_REST}

## Move the files
mv ${RESULTS_FILE} ./results/${date}/${TEST_NAME}/result_${TEST_VARIABLE_NUMBER}_${cur_time}.txt
#===================================
