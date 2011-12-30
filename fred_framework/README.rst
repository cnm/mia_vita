README
======

File structure:

.
├── chains.c                        - Chains are an equivalent to the chain in iptables
├── chains.h
├── compile.sh                      - Script to compile things
├── error_msg.c
├── filter.c                        - Filters are the lowest class. THey are the one which see if a packet matches
├── filter.h
├── hooks.c
├── hooks.h
├── interceptor.h
├── interceptor_manager.c
├── interceptor_manager.h
├── interceptors
│   ├── aggregation                 - Chain responsible to do the aggregation
│   └── deaggregation
├── interceptor_shell
├── klist.c
├── klist.h
├── main.c                          - Main for the interceptor framework kernel module
├── proc_entry.c                    - Creates a proc device for communication
├── proc_entry.h
├── proc_registry.c                 - 
├── proc_registry.h
├── rule.c                          - Rules contain chains
├── rule.h
├── rule_manager.c
├── rule_manager.h
└── uncompress_kernel_image.bash
