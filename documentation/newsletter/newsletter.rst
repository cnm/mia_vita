A Wireless Sensor Network for Monitoring Volcanic Tremors
Teresa Vazão, MIAVITA team member,
Lisbon Tech, Lisboa, Portugal
Email: tvazao[at]tagus.inesc-id.pt

Deployment of short duration complex experimental setups with a significant number of sensor nodes dispersed over a wide geographical region is unusual. The reason for such lack of experiments is either because it is to difficult to install/maintain the several nodes or because the deployed equipment is very expensive and risk of damage is high.
A promising solution appeared with the emergence of Wireless Sensor Networks. This technology allowed the ad hoc deployment of low cost systems which can be easily deployed and automatically interconnected, creating networks which are used to transport a broad range of information.

For the MIA-VITA project, the INESC-ID team located in Lisbon developed a Wireless Sensor Network solution suited for volcanological experiments. As shown on Figure 1, the network of the developed solution is composed of 13 nodes. The green nodes, named sensor nodes, simply collect and send data. The central node, named the sink node, is a special node as it is responsible to receive data from all other nodes and relay this information to the Remove Laboratory.

The hardware architecture for each each node is present on Figure 2. It is possible to connect each node to one of two geophone sensors models: uni-axial or tri-axial. The measurements reported by these sensors are then acquired by the Data Acquisition System, which is responsible to accurately timestamp the data and to convert the analogue signal to a digital representation. In terms of processing, each node is installed with a ARM processor base system suited for embedded solutions. For communication each node possesses a Wireless 802.11b compatible USB card.

.. figure:: architecture.png
    :width: 300

Using a small team of two or three persons it is possible to deploy the entire network in a short amount of time. As it is possible to see on Figure 3 the developed solution is easy to carry, and installation restrains to securing the geophone and metal case to the ground, making sure the wireless antenna and GPS device are facing up. No human intervention is necessary for configuring the network. If units are to be left running for days, then it is necessary to connect nodes to a 12V battery or an optional solar panel.

.. figure:: caixaNoTerreno.jpg
    :width: 200

After the equipment is deployed it is possible to check the device status on the terrain either by four status lights present on the metal case, or by connecting via wireless to the node using a normal laptop or compatible tablet. The user in this case is presented with a web page with the node/network status and with the most recent collected samples as shown on Figure 4.

.. figure:: visualization.jpg
    :width: 300
