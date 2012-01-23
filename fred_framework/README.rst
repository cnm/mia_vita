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
