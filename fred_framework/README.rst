README
======

File structure:

.
├── chains.c
├── chains.h
├── compile.sh
├── error_msg.c
├── filter.c
├── filter.h
├── hooks.c
├── hooks.h
├── interceptor.h
├── interceptor_manager.c
├── interceptor_manager.h
├── interceptors
│   ├── aggregation
│   │   ├── aggregation.c
│   │   ├── byte_buffer.c
│   │   ├── byte_buffer.h
│   │   └── Makefile
│   ├── deaggregation
│   │   ├── deaggregation.c
│   │   ├── injection_thread.c
│   │   ├── injection_thread.h
│   │   ├── Makefile
│   │   ├── spin_lock_queue.c
│   │   └── spin_lock_queue.h
│   ├── miavita_packet.h -> ../../kernel_sender/miavita_packet.h
│   └── utils.h
├── interceptor_shell
│   ├── cmd_line_parser.c
│   ├── cmd_line_parser.h
│   ├── macros.h
│   ├── Makefile
│   ├── mkrule.c
│   ├── operations.c
│   ├── operations.h
│   └── rmrule.c
├── klist.c
├── klist.h
├── main.c
├── Makefile
├── proc_entry.c
├── proc_entry.h
├── proc_registry.c
├── proc_registry.h
├── rule.c
├── rule.h
├── rule_manager.c
├── rule_manager.h
└── uncompress_kernel_image.bash


