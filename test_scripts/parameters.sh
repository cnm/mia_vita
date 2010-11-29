############ Parameters ######
ROUTING="STATIC BATMAN"
SAMPLE_PER_SECOND="25 50"
SEED=`seq 1 5`             # Number of times a test will run
N_PACKETS=100

############# TIMERS #########
T_INITIAL_HUMAN=5
T_BATMAN_WAIT=5
T_LIMIT_TEST_TIME=30

############# TIMERS in scripts #########
T_NET_DELAY=2
T_NODE_DELAY=2
T_JITTER=20
T_CAN=5
T_REST=5
