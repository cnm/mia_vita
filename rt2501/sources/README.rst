Download the driver
===================
Url: ftp://ftp.embeddedarm.com/ts-arm-sbc/ts-7500-linux/sources/

File: wifi-g-usb-2_rt2501usb-sources.tar.gz

File Structure
==============


.
Module
├── assoc.c                 -> Just corrected two "sprintf(p, "\0");" which did nothing
├── filter_chains.c         -> TODO
├── filter_chains.h
├── Makefile -> Makefile.6
├── miavita_packet.h -> ../../../kernel_sender/miavita_packet.h
├── mlme.c                  -> Just taken the variable RxSignal used for debug
├── proc_filters.c          -> TODO
├── proc_filters.h
├── rtmp_info.c             -> Fixed the passes SSID
├── rtmp_main.c             -> Added code to initialize (and destroy) the filters
├── rtusb_bulk.c            -> Adde Explicit cast and debug message. Function which delivers to the controller (RTUSBBulkOutDataPacket)
├── rtusb_data.c            -> rts_cts_frame_duration and REPORT_ETHERNET_FRAME_TO_LLC (Function which delivers to ethernet)
├── sync_proto.c            -> Function which sync in and sync out the packet
├── sync_proto.h
├── user
│   ├── Makefile
│   ├── mkfilter.c
│   ├── proc_write.c
│   ├── proc_write.h
│   └── rmfilter.c
