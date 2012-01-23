README
======

File structure::

    .
    ├── chains.c                        - Chains are an equivalent to the chain in iptables
    ├── chains.h
    ├── compile.sh                      - Script to compile things
    ├── error_msg.c
    ├── filter.c                        - Filters are the lowest class. THey are the one which see if a packet matches
    ├── filter.h
    ├── hooks.c                         - The hooks are the "windows" for filters to match a packet (by source, destination, ports)
    ├── hooks.h
    ├── interceptor.h
    ├── interceptor_manager.c
    ├── interceptor_manager.h
    ├── interceptors
    │   ├── aggregation                 - Chain responsible to do the aggregation
    │   └── deaggregation
    ├── interceptor_shell
    ├── klist.c                         - List structures to be used in the kernel
    ├── klist.h
    ├── main.c                          - Main for the interceptor framework kernel module
    ├── proc_entry.c                    - Creates a proc device for communication
    ├── proc_entry.h
    ├── proc_registry.c                 - 
    ├── proc_registry.h
    ├── rule.c                          - Rules contain chains
    ├── rule.h
    ├── rule_manager.c                  - 
    ├── rule_manager.h
    └── uncompress_kernel_image.bash

How to
======

Compiling
---------

First the code needs to be compiled. This should be done using the script ``compile.bash``::

    ./compile.bash

This script needs the correct kernel to work. By default it uses the kernel listed by the command 
``uname -r``, but it is possible to specify another. To do this just add the flag ``-k``::

    ./compile.bash -k /lib/modules/2.6.24.4/build/

Also, if you just need to clean the modules without compiling them, use the flag ``-c``::

    ./compile.bash -c

If you're developing new interceptors anda just want to compile the currently working code, add the flag ``-r``
to the script invocation and it will continue compiling the code even if there are errors.

Using the modules
-----------------

After the ``compile.bash`` script is run, a folder named ``modules`` will be created with each ``.ko`` file
that was compiled by the script. (Note: the folder may be renamed at compile time, by specifying the flag ``-o``
and the name of the new folder, when running the script ``compile.bash``).

At least one module should be present, which is ``interceptor_framework.ko``. This is the main module, which
provides the basic structure to use interceptors. Just insert it into the kernel::

    insmod modules/interceptor_framework.ko

If the aggregation module is also present then you should insert it as well. However, this module can be
parameterized at insertion time. Just run ``modinfo aggregate.ko`` and check out the modules parameters. Then,
insert it using the values you desire for such parameters.

Step by Step Example
--------------------

Imagine a network with 3 nodes, such that node A sends data to node C, which is routed through node B::

    A----------->B----------->C

Now suppose that the IP numbers of each node is as follows::

+------------+-------------+
|    Node    |      IP     |
+------------+-------------+
|     A      | 192.168.0.1 |
+------------+-------------+
|     B      | 192.168.0.2 |
+------------+-------------+
|     C      | 192.168.0.3 |
+------------+-------------+

Also, there exists an application sending data in node A and an application receiving data in node C. Both are
using UDP and binded to port ``57843``.

The idea is to make node A aggregate packets, so the number of frames sent to the network is reduced. For this 
need to insert the ``aggregate.ko`` module in node A and ``deaggregate.ko`` module in node C. Note that node B
does not need any modules (not even the ``interceptor_framework.ko``).

Inserting the ``deaggregate.ko`` module is pretty straight forward::

    insmod modules/deaggregate.ko

Inserting the ``aggregate.ko`` file requires a little more attention. Because aggregation can be done to IP
packets and UDP packets, we need to choose wisely. In this scenario is best to use aggregation at UDP level
because it has less overhead than the IP aggregation and therefore an aggregated UDP packet will contain more
packets than an aggregated IP packet. Also, note that there is no difference in terms of traffic flow if we 
aggregate UDP or IP packets. Thus UDP is the best choice here.

However, we need to tell the ``aggregate.ko`` module that we want only to aggregate UDP packets. This is done
through module parameters. Issuing ``modinfo`` in a module yields such parameters::

    modinfo modules/aggregate.ko

By default the module will only aggregate IP packets with an aggregated packet size of 512 bytes. In our case
we want a command like::

    insmod modules/aggregate.ko aggregate_ip_packets=0 aggregate_app_packets=1 \ 
                                aggregate_app_packets_buffer_size=<some_size_lees_than_mtu> \
                                bind_ip="192.168.0.1"

What the above command does is to load the module telling it not to aggregate IP packets, but aggregate UDP 
packets instead. It also tells the size of the aggregated packet, not counting the extra IP header.

Last, but very important, is the ``bind_ip`` parameter. This is used to for the source address of an aggregated
packet. Why is this necessary? The module spawns a kthread which from time to time will flush the aggregation
buffers. This means that aggregated packets will be created and sent through the network. However, a source
address must be specified in the new IP header. The address provided in ``bind-ip`` will be used. Note that
it is also possible to specify the ``flush_timeout``.

Ok, so now that the modules are inserted in nodes A and C we need to specify the traffic flow to aggregate.
This is necessary to avoid aggregation of every packet that crosses the network. Therefore, we need to create
rules which will tell the aggregation deaggregation modules, which packets should be processed. This is
similar to iptables.

In the folder ``interceptor_shell`` there is source code which creates these rules. So just use the compiler
you want and issue a ``make`` to compile the code. Two binaries will be created - ``mkrule`` and ``rmrule``.

Just be sure you use these commands after inserting the modules.

So to create rules we use the ``mkrule`` command. In node A we need to issue::

    mkrule -n aggregation -da 192.168.0.3 -dp 57843

This will tell the interceptor framework to create a rule for the aggregation interceptor, for packets with C's
destination address and 57843 as destination port.

Similar in node C we need to issue a command to deaggregate traffic::

    mkrule -n deaggregation -da 192.168.0.3 -dp 57843

This will tell the interceptor framework to create a rule for the deaggregation interceptor, for packets with
C's destination address and 57843 as destination port.

This is all what is needed to aggregate a traffic flow.

Step by Step with a more complex network
----------------------------------------

If you didn't read the step by step above, you will not understand this example. Now imagine that there is a
fourth node in the network such that it looks like this::

    A--------------->B-------------->C
                     ^
                     |
                     |
                     D

As opposed to the previous scenario, in this one it makes more sense to aggregate IP packets in node B. This
will reduce the overhead in the link ``B--->C``.

In this case, only node B needs to load the aggregation module and it should be as follows::

    insmod modules/aggregate.ko aggregate_ip_buffer_size=<some_size_lees_than_mtu> \
                                bind_ip="192.168.0.2"

The rules to aggregate traffic can be the same as the ones provided in the previous example. However, they
must be issued in node B and node C. Obviously node C needs the deaggregation module as well.

Addons
======

Because there is no easy way to print an error message associated with an error code from a kernel module, I 
just print the error code. However, I've also provided a program in user land which given an error code prints
the error message associated with it. This program is called ``emsg`` and can be compiled by issuing::

    make error_msg

Be careful because the command ``./compile.bash -c`` also cleans this program.

After getting the error from syslog, you can just use it as::

    ./emsg <error_code>

For example::

    ./emsg 90
    Message to long
