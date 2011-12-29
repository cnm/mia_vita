File structure
==============

This is the file structure:

.
├── compile.bash                - Use this to compile (because of dependencies in modules)
├── miavita_packet.h            - It is here that the MIAVITA message structure is defined
├── sender_kthread.c            - Module which sends packets and times them (GPS timing option with -D__GPS__ flag)
└── user                        - Userland program to receive the MIA Vita packets and to write them to two file (json and raw)
    ├── ...
    ...
