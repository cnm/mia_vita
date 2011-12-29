Download the driver
===================
Url: ftp://ftp.embeddedarm.com/ts-arm-sbc/ts-7500-linux/sources/

File: wifi-g-usb-2_rt2501usb-sources.tar.gz

File Structure
==============

 rt2501/sources/Module/Makefile.6        |   28 +-
 rt2501/sources/Module/assoc.c           | 1928 +++++++-------
 rt2501/sources/Module/deploy.bash       |   11 -
 rt2501/sources/Module/filter_chains.c   |   67 -
 rt2501/sources/Module/filter_chains.h   |   32 -
 rt2501/sources/Module/miavita_packet.h  |    1 -
 rt2501/sources/Module/mlme.c            |    4 +
 rt2501/sources/Module/proc_filters.c    |  201 --
 rt2501/sources/Module/proc_filters.h    |   23 -
 rt2501/sources/Module/rt_config.h       |    7 +-
 rt2501/sources/Module/rtmp_info.c       |  243 +-
 rt2501/sources/Module/rtmp_main.c       |   19 +-
 rt2501/sources/Module/rtusb_bulk.c      |  228 +-
 rt2501/sources/Module/rtusb_data.c      | 4311 ++++++++++++++++---------------
 rt2501/sources/Module/sync_proto.c      |  207 --
 rt2501/sources/Module/sync_proto.h      |   19 -
 rt2501/sources/Module/user/Makefile     |   15 -
 rt2501/sources/Module/user/mkfilter.c   |   50 -
 rt2501/sources/Module/user/proc_write.c |   82 -
 rt2501/sources/Module/user/proc_write.h |    8 -
 rt2501/sources/Module/user/rmfilter.c   |   13 -
 rt2501/sources/README.rst               |   72 -



.
├── Module
│   ├── assoc.c
│   ├── deploy.bash
│   ├── filter_chains.c
│   ├── filter_chains.h
│   ├── Makefile -> Makefile.6
│   ├── miavita_packet.h -> ../../../kernel_sender/miavita_packet.h
│   ├── mlme.c
│   ├── proc_filters.c
│   ├── proc_filters.h
│   ├── rtmp_info.c
│   ├── rtmp_main.c
│   ├── rtusb_bulk.c
│   ├── rtusb_data.c
│   ├── sync_proto.c
│   ├── sync_proto.h
│   ├── user
│   │   ├── Makefile
│   │   ├── mkfilter.c
│   │   ├── proc_write.c
│   │   ├── proc_write.h
│   │   └── rmfilter.c
