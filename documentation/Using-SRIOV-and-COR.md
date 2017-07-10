# Setup to use SRIO-V with Clear Containers

Single Root IO Virtualization enables splitting a physical device into
virtual functions (VF) which enables direct pass through to virtual machines or
containers.  For Clear Containers, we have enabled a CNM plugin and necessary
changes to facilitate making use of SRIO-V for network based devices.

## Install the SRIO-V Docker\* plugin

To create a network with VFs associated with it which can be passed to
Clear Containers, install a SRIOV Docker plugin. The created network will be
based on a physical function (PF) device, and can create 'n' containers, where
'n' is the number of VFs associated with the PF

To install the plugin, you can follow the [plugin installation instructions](https://github.com/clearcontainers/sriov).


## Host setup for SRIO-V

You may need to rebuild your system's kernel in order to disable VFNOIOMMU in
the kernel config and potentially add a PCI quirk for your NIC. If not, you are
lucky and can move to next section. First, I'll describe how to assess if
changes are needed, and then describe how to make these changes.

### IOMMU Groups and PCIe Access Control Services
Taking a look at how the iommu groups are setup on your host system can help
provide information on whether or not your NIC is setup appropriately with
respect to PCIe Access Control Services. More specifically, if the PCI bridge is
within the same iommu-group as your NIC, then this is an indication that either
your device doesn't support ACS or that it doesn't share this capability
appropriately by default.

For example, when you run the following command, if all is setup properly, you
should have the PCI for each ACS enabled NIC port in its own iommu_group. If
you don't see any output when running the following, then you likely need to
update your host's kernel config to disable VFNOIOMMU.

```
find /sys/kernel/iommu_groups/ -type l
```

For more details, see http://vfio.blogspot.com/2014/08/iommu-groups-inside-and-out.html

### Updating the Host Kernel

These directions are provided as an example, and are based on Ubuntu 16.04. The
user will need to do equivalent for their distribution.

1.  Grab kernel sources

```
sudo apt-get install linux-source-4.10.0
sudo apt-get install linux-headers-4.10.0
cd /usr/src/linux-source-4.10.0/
sudo tar -xvf linux-source-4.10.0.tar.bz2
cd linux-source-4.10.0
sudo apt-get install libssl-dev
```

2. Check the config and update if necessary

```
sudo cp /boot/config-4.8.0-36-generic .config
sudo make olddefconfig #and verify resulting .config does not have NOIOMMU set; ie: # CONFIG_VFIO_NOIOMMU is not set
```

3.  If necessary, add PCI Quirk for SRIOV NIC

Now, depending on how your NIC describes its ACS capabilities, you may need to add
a quirk in order to indicate that the given NIC does properly support ACS.  An
example is given below, but your mileage will vary (at the very least, check your
PCI-ID).

```
modify drivers/pci/quirks.c:
line 4118:
static const u16 pci_quirk_intel_pch_acs_ids[] = {
+        0x0c01,
        /* Ibexpeak PCH */
        0x3b42, 0x3b43, 0x3b44, 0x3b45, 0x3b46, 0x3b47, 0x3b48, 0x3b49,
```

4.  Build and install the kernel:

```
sudo make -j #and go get coffee
sudo make modules -j 3
sudo make modules_install
sudo make install
```

5.  Edit grub to enable intel-iommu:

```
edit /etc/default/grub and add intel_iommu=on to cmdline:
- GRUB_CMDLINE_LINUX=""
+ GRUB_CMDLINE_LINUX="intel_iommu=on"
sudo update-grub
```

6.  Reboot and verify
Host system should be ready now -- reboot and verify that expected cmdline and
kernel version is booted (look at /proc/cmdline and /proc/version)

```
sudo reboot
```

## Setting up SRIO-V Device

All of the prior sections are needed once to prepare the SRIOV host systems. The following will be needed per boot
in order to facilitate setting up a physical device's virtual functions.

For SRIOV, a physical device can create up to `sriov_totalvfs` virtual functions (VFs).  Once created, you cannot
grow or shrink the number of VFs without first setting it back to zero.  Based on this, it is expected that you
should set the number of VFs from a physical device  just once.

1. Add vfio-pci device driver

vfio-pci is a driver which is used to reserve a VF PCI device.  Add it:
```
sudo modprobe vfio-pci
```
2. Find our NICs of interest

Find PCI details for the NICs in question:
```
sriov@sriov-1:/sys/bus/pci$ lspci | grep Ethernet
00:19.0 Ethernet controller: Intel Corporation Ethernet Connection I217-LM (rev 04)
01:00.0 Ethernet controller: Intel Corporation Ethernet Controller 10-Gigabit X540-AT2 (rev 01)
01:00.1 Ethernet controller: Intel Corporation Ethernet Controller 10-Gigabit X540-AT2 (rev 01)
```
In our case, both 01:00.0 and 01:00.1 are the two ports on our x540-AT2 card that we'll use.  You can use
lshw to get further details on the controller and verify it indeed supports SRIO-V.

3.  Check how many VFs we can create:
```
$ sriov@sriov-1:~$ cat /sys/bus/pci/devices/0000\:01\:00.0/sriov_totalvfs
63
$ sriov@sriov-1:~$ cat /sys/bus/pci/devices/0000\:01\:00.1/sriov_totalvfs
63
```
4.  Create VFs:

Create virtual functions by editing sriov_numvfs. In our example, let's just create one
per physical device.  Note, this eliminates the usefulness of SRIOV, and is just done for
simplicity in this example so you needn't look at 128 virtual devices.

```
root@sriov-1:/home/sriov# echo 1 > /sys/bus/pci/devices/0000\:01\:00.0/sriov_numvfs
root@sriov-1:/home/sriov# echo 1 > /sys/bus/pci/devices/0000\:01\:00.1/sriov_numvfs
```

5.  Verify that these were added to the host:

```
root@sriov-1:/home/sriov# lspci | grep Ethernet | grep Virtual
02:10.0 Ethernet controller: Intel Corporation X540 Ethernet Controller Virtual Function (rev 01)
02:10.1 Ethernet controller: Intel Corporation X540 Ethernet Controller Virtual Function (rev 01)
```
6.  Assign MAC address to each VF

Depending on the NIC being used, you may need to explicitly set the MAC address for the Virtual Function device in order to guarantee that the address is consistent on the host and when passed through to the guest.  For example,

```
ip link set <pf> vf <vfidx> mac <my made up mac address>
```

## Example: Launch a Clear Container using SRIO-V

1.  Build and start SRIOV plugin

This assumes you already have GOPATH set in your environment.
```
sriov@sriov-2:~$ sudo mkdir /etc/docker/plugins
sriov@sriov-2:~$ sudo cp go/src/github.com/clearcontainers/sriov/sriov.json /etc/docker/plugins/
cd /go/src/github.com/clearcontainers/sriov
go build
sudo ./sriov &
```
2.  Create docker network

To create the SRIOV based docker network, a subnet, vlanid and physical interface are required.

```
sudo docker network create -d sriov --internal --opt pf_iface=enp1s0f0 --opt vlanid=100 --subnet=192.168.0.0/24 vfnet


E0505 09:35:40.550129    2541 plugin.go:297] Numvfs and Totalvfs are not same on the PF - Initialize numvfs to totalvfs
ee2e5a594f9e4d3796eda972f3b46e52342aea04cbae8e5eac9b2dd6ff37b067
```

3.  Start containers and test connectivity

To start a container making use of SRIOV:

```
sriov@sriov-2:~$ sudo docker run --runtime=cor --net=vfnet --ip=192.168.0.10 -it alpine
```

If you have two machines with SRIOV enabled NICs connected back to back and each
has a network with matching vlan-id created, you can test connectivity as follows:

Machine #1:
```
sriov@sriov-2:~$ sudo docker run --runtime=cor --net=vfnet --ip=192.168.0.10 -it mcastelino/iperf iperf3 -s
```
Machine #2:
```
sriov@sriov-1:~$ sudo docker run --runtime=cor --net=vfnet --ip=192.168.0.11 -it mcastelino/iperf iperf3 -c 192.168.0.10
```
