File structure
==============

This is the file structure::

  .
  ├── compile.bash                - Use this to compile (because of dependencies in modules)
  ├── miavita_packet.h            - It is here that the MIAVITA message structure is defined
  ├── sender_kthread.c            - Module which sends packets and times them (GPS timing option with -D__GPS__ flag)
  └── user                        - Userland program to receive the MIA Vita packets and to write them to two file (json and raw)
      ├── ...
      ...

Objective
=========

    Kernel sender is a module application which reads data from the buffer created at interruption/proc_entry.c and sends it to the other nodes.

Drawing
=======

Logic behind the synchronization::


 +---------+                                             +---------+                         +---------+
 |   App   |  (1)        (Kernel Module)                 |   App   |                     (1) |   App   |
 |---------|   |                                         |---------|                      ↑  |---------|
 |  Trans  |   |         (UDP)                           |  Trans  |                      |  |  Trans  |
 |---------|   |                                         |---------|                      |  |---------|
 |  Inter  |   |         (IP)                            |  Inter  |                      |  |  Inter  |
 |---------|   |                                         |---------|                      |  |---------|
 |         |   ↓                                       ↔ |         | ↔                    |  |         |
 |  Link   |  (1->2)     (MAC - Kernel Module)    (2->1) |  Link   | (1->2)           (2->1) |  Link   |
 |         |   --------------------------------------->  |         | --------------------->  |         |
 +---------+                                             +---------+                         +---------+

 1    )  Happens at the interruption at proc_entry.c  (function: read_four_channels in interruption/fpga.c)
 1-2  )  Happens in the rt2501 module                 (function: synch_out_data_packet in rt2501/sources/Module/sync_proto.c)
 2-1  )  Happens at the rt2501 module                 (function: synch_in_data_packet  in rt2501/sources/Module/sync_proto.c)

