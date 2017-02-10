# Setup to run VPP

The Data Plane Development Kit (DPDK) is a set of libraries and drivers for
fast packet processing. The Vector Packet Processing (VPP) is a platform
extensible framework which provides out-of-the-box production quality
switch and router functionality. VPP is a high performance packet-processing
stack which can run on commodity CPUs. Enabling VPP with DPDK support can
yield significant performance improvements over a Linux\*-bridge providing a
switch with DPDK vhost-user ports.

For more information about VPP visit their [wiki](https://wiki.fd.io/view/VPP).

## Install VPP

Follow the [VPP installation instructions](https://wiki.fd.io/view/VPP/Installing_VPP_binaries_from_packages).

After a successful installation, your host system is ready to start
connecting Intel® Clear Containers with VPP bridges.

### Install the VPP Docker\* plugin

To create a network and connect Clear Containers easily to that network via
Docker, install a VPP Docker plugin.

To install the plugin, you can follow the [plugin installation instructions](https://github.com/clearcontainers/vpp).

This VPP plugin allows the creation of a VPP network. Every container added
to this network will be connected via an L2 bridge-domain provided by VPP.

## Example: Launch two Clear Containers using VPP

To use VPP, use Docker to create a network which uses the OVS-DPDK switch.
For example:

```
$ sudo docker network create -d=vpp --ipam-driver=vpp --subnet=192.168.1.0/24 --gateway=192.168.1.1  vpp_net
```

Test connectivity by launching two containers, assuming Docker is setup to
use this branch's runtime:

```
$ sudo docker run --net=vpp_net --ip=192.168.1.2 --mac-address=CA:FE:CA:FE:01:02 -it debian bash -c "ip a; ip route; sleep 300"

$ sudo docker run --net=vpp_net --ip=192.168.1.3 --mac-address=CA:FE:CA:FE:01:03 -it debian bash -c "ip a; ip route; ping 192.168.1.2"
```

These commands setup two Clear Containers connected via a VPP L2 bridge
domain. The first of the the VMs displays the networking details and then
sleeps providing a period of time for it to be pinged. The second
VM displays its networking details and then pings the first VM, verifying
connectivity between them.

After verifying connectivity, cleanup with the following commands:

```
$ sudo docker kill $(sudo docker ps --no-trunc -aq)
$ sudo docker rm $(sudo docker ps --no-trunc -aq)
$ sudo docker network rm vpp_net
$ sudo service vpp stop
$ sudo service vpp start
```
