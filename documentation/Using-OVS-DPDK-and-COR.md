# Using OVS-DPDK with Clear Containers

The Data Plane Development Kit, or DPDK, is a set of libraries and drivers for
fast packet processing. Open vSwitch, or OVS, is an open-source implementation
of a distributed multilayer switch. Enabling DPDK support within OVS can yield
significant performance improvements over a linux-bridge and also provides a
switch with DPDK vhost-user ports.

## Installing DPDK and OVS on the host system

The versions of OVS and DPDK are tightly coupled. Versions available via
distributions are often very out of date. The following instructions assume you
are building and configuring from the source repos. Update the packages listed
regularly to ensure your Intel® Clear Containers benefit from performance
improvements, new features, and CVE security fixes.

### Prerequisites

The following packages are needed on your host to build OVS and DPDK.

* autoconf
* automake
* kernel-common
* libpcap-dev
* libtool
* python
* python-six

For example, install the packages on an Ubuntu host system with the following
command:

```
$ sudo apt-get -y install autoconf automake kernel-common \
    libpcap-dev libtool python python-six
```

### Installing the DPDK

1. Download the DPDK source code and move into the folder:

   ```
   $ git clone http://dpdk.org/git/dpdk -b v16.11 $HOME/dpdk
   $ cd $HOME/dpdk
   ```

2. Set the environmental variables needed for building:

   ```
   $ export RTE_SDK=$(pwd)
   $ export RTE_TARGET=x86_64-native-linuxapp-gcc
   ```

3. Build the DPDK and copy the binaries to the appropriate folder:

   ```
   $ make -j install T=$RTE_TARGET DESTDIR=install EXTRA_CFLAGS='-g'
   $ sudo cp -f install/lib/lib* /lib64/
   ```

4. Set the environmental variable to the correct folder for the OVS configuration:

   ```
   $ export DPDK_BUILD_DIR=x86_64-native-linuxapp-gcc
   ```

### Installing the OVS

Once the DPDK setup completes with out issue, build and configure the OVS with
the following steps:

1. Download the OVS source code and move into the folder:

   ```
   $ git clone https://github.com/openvswitch/ovs -b branch-2.6 $HOME/ovs
   $ cd $HOME/ovs
   ```

2. Run the `boot.sh` script:

   ```
   $ ./boot.sh
   ```

3. Provide the following configuration values:

   ```
   $ ./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var \
       --with-dpdk="$RTE_SDK/$DPDK_BUILD_DIR"  --disable-ssl --with-debug  CFLAGS='-g'
   ```

4. Install the OVS with the following Make command:

   ```
   $ sudo -E make install -j
   ```

## Running OVS-DPDK on host system

Once both, the OVS and the DPDK, are installed without error, start 
`ovsdb-server` and `ovs-vswitchd`. The Open_vSwitch configuration settings vary
depending on the hardware OVS runs on, for example `lcore-mask` and 
`socket-mem`. The following is an example configuration which should be 
adjusted for your setup:

1. Create a clean folder for OVS and remove other configurations:

   ```
   $ sudo mkdir -p /var/run/openvswitch
   $ sudo killall ovsdb-server ovs-vswitchd
   $ rm -f /var/run/openvswitch/vhost-user*
   $ sudo rm -f /etc/openvswitch/conf.db
   ```

2. Setup the OVS database components:

   ```
   $ export DB_SOCK=/var/run/openvswitch/db.sock
   $ sudo -E ovsdb-tool create /etc/openvswitch/conf.db /usr/share/openvswitch/vswitch.ovsschema
   $ sudo -E ovsdb-server --remote=punix:$DB_SOCK --remote=db:Open_vSwitch,Open_vSwitch,manager_options --pidfile --detach
   ```

3. Configure OVS's use of the DPDK:

   ```
   $ sudo -E ovs-vsctl --no-wait init
   $ sudo -E ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-lcore-mask=0x3
   $ sudo -E ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-socket-mem=1024,0
   $ sudo -E ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-init=true
   ```

4. Configure and mount the `hugepages`:

   ```
   $ sudo sysctl -w vm.nr_hugepages=4096
   $ sudo mount -t hugetlbfs -o pagesize=2048k none /dev/hugepages
   ```

5. Start OVS with the appropriate options:

   ```
   $ sudo -E ovs-vswitchd unix:$DB_SOCK --pidfile --detach --log-file=/var/log/openvswitch/ovs-vswitchd.log
   ```

6. Review the status of the OVS process:

   ```
   $ ps -ae | grep ovs
   ```

On step **3** of the example, the following configuration options were set for
the DPDK:

* **dpdk-lcore-mask**: Specifies the CPU cores where DPDK threads should be
  spawned. The example assigns 0x3 which identifies cores 0 and 1. The setup of
  the host system determined this decision. Only after monitoring the lspci
  output to determine which cores are tied to the same NUMA node, could the
  decision be made.

* **dpdk-socket-mem**: Specifies a list of comma separated values
  of memory in MB to preallocate from the hugepages on specific sockets. The
  example requests 1024 MB from socket 0; since cores 0 and 1 are also located
  on socket 0.

* **dpdk-init**:  Specifies whether OVS should initialize and support DPDK
  ports.

On step **4** of the example, the number of hugepages mounted at
`/dev/hugepages` was determined based on the size of memory allocated to each
VM and the size allocated to OVS itself. Our example will launch two Clear
Containers using OVS-DPDK. Each container will preallocate 2GB per container
from hugepages and the OVS-DPDK requires 1GB. Thus, 5GB was the minimum amount
of memory required to reserve.

Once the steps were followed and modified as necessary, the host system is
ready to start creating DPDK enabled OVSes.

## Installing the OVSDPDK Docker\* plug-in

To easily create a network and connect the Clear Containers to it via Docker,
an OVS-DPDK Docker plug-in must be installed.

Details on installing the plug-in are found at
https://github.com/clearcontainers/ovsdpdk

## Launching two Clear Containers using OVS-DPDK

1. To use of OVS-DPDK, a network is needed. Use Docker to create a network using the OVS-DPDK switch, for example:

   ```
   $ sudo docker network create -d=ovsdpdk --ipam-driver=ovsdpdk --subnet=192.168.1.0/24 --gateway=192.168.1.1  --opt "bridge"="ovsbr" ovsdpdk_net
   ```

2. To verify the OVS switch was created, search for the OVS bridge, `ovsbr`, in the output generated by the following command:

   ```
   $ sudo ovs-vsctl show
   ```

3. To test the connectivity, launch two containers. The following commands
   assume Docker is setup to use this branch's runtime.

   1. Launch the first Clear Container, display its networking details, and set
      it to sleep providing a window for it to be pinged:

      ```
      $ sudo docker run --net=ovsdpdk_net --ip=192.168.1.2 --mac-address=CA:FE:CA:FE:01:02 -it debian bash -c "ip a; ip route; sleep 300"
      ```

   2. Launch the second Clear Container, display its networking details, and
      ping the first container.

      ```
      $ sudo docker run --net=ovsdpdk_net --ip=192.168.1.3 --mac-address=CA:FE:CA:FE:01:03 -it debian bash -c "ip a; ip route; ping 192.168.1.2"
      ```

   Once the ping is successful, connectivity between the containers is verified.

4. After verification, cleanup with the following commands:

   ```
   $ sudo docker kill $(sudo docker ps --no-trunc -aq)
   $ sudo docker rm $(sudo docker ps --no-trunc -aq)
   $ sudo docker network rm ovsdpdk_net
   ```

