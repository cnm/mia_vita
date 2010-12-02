Brief Summary
=============


Scenarios
=========
Note1: C - Client (Collector), M - Middle, S - Server (Sink)

Scenario 1
----------

C1 ------ S1
    40m

Direct connection between the client and the server.

Scenario 2
----------

C1 ------ M1 ------- S1
    40m        40m

Connection with the client and the server with an intermediate node.
There should not be any possible direct communication between the client and server.


Instructions
============

Global startup instructions
---------------------------

* Connect the wireless card to the device
* Start the respective nodes identified by (check the scenario to see which nodes should be turned on):
  * C1: MV-25
  * M1: MV-26
  * S1: MV-27
* Wait around 30 seconds
* Connect to it via telnet
  * User: root password: cnm
* Check if the wifi device is blinking

Client (collector) instructions
-------------------------------

Scenario 1
..........

* Go to the directory:
  /root/test_scripts
* run the command:
  nice -20 sh client.sh ./sampler 40
* Wait around 10 seconds until the red light is on
* **Synchronously** with the sink S1 node, press enter
* Wait 10 minutes (+-) until the red light appears.
* Repeat the previous two steps 11 times

Scenario 1
..........

* Go to the directory:
  /root/test_scripts
* run the command:
  nice -20 sh client.sh ./sampler 40
* Wait around 10 seconds until the red light is on
* **Synchronously** with the sink S1 node, press enter
* Wait 10 minutes (+-) until the red light appears.
* Repeat the previous two steps 5 more times

Scenario 2
..........

* The client should say to its paused waiting for input
* **Synchronously** with the sink S1 node, press enter
* Wait 10 minutes (+-) until the red light appears.
* Repeat the previous two steps 5 more times.

Sink instructions
-----------------

* Go to the directory:
  /root/test_scripts
* run the command:
  nice -20 sh server.sh ./sampler 40
* Wait around 10 seconds until the red light is on
* **Synchronously** with the client C1 node, press enter
* Wait 10 minutes (+-) until the red light appears.
* Repeat the previous two steps 5 more times

Scenario 2
..........

* Turn on the middle node (follow its instructions)
* The client should say to its paused waiting for input
* **Synchronously** with the client C1 node, press enter
* Wait 10 minutes (+-) until the red light appears.
* Repeat the previous two steps 5 more times.

Middle instructions
-------------------

Scenario 1
..........

* Just make sure it is off

Scenario 2
..........

* Insert the USB wireless device
* Turn on the ARM device
* Go to the directory:
  * /root/test_scripts

* run the command:
  * nice -20 sh server.sh ./sampler 40
