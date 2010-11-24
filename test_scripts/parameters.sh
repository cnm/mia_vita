############ Parameters ######
ROUTING="STATIC BATMAN"
SAMPLE_PER_SECOND="25 50"
SEED=`seq 1 30`             # Number of times a test will run
N_PACKETS=100

############# TIMERS #########
T_INITIAL_HUMAN=10
T_BATMAN_WAIT=5
T_LIMIT_TEST_TIME=60

############# TIMERS in scripts #########
T_NET_DELAY=2
T_NODE_DELAY=2
T_JITTER=5
T_CAN=5
T_REST=5
