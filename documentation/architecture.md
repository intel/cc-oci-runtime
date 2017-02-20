# Clear Containers Architecture

* [Overview](#overview)
  * [Hypervisor](#hypervisor)
  * [Hyperstart](#hyperstart)
  * [Runtime](#runtime)
    * [create](#create)
    * [start](#start)
    * [exec](#exec)
    * [kill](#kill)
    * [delete](#delete)
  * [Proxy](#proxy)
  * [Shim](#shim)
  * [Networking](#networking)

## Overview

This is an architectural overview of Clear Containers, based on the 2.1 release.

The [Clear Containers runtime (cc-oci-runtime)](https://github.com/01org/cc-oci-runtime)
complies with the [OCI](https://github.com/opencontainers) specifications and thus
works seamlessly with the [Docker Engine](https://www.docker.com/products/docker-engine)
pluggable runtime architecture. In other words, one can transparently replace the
[default Docker runtime (runc)](https://github.com/opencontainers/runc) with `cc-oci-runtime`.

![Docker and Clear Containers](https://github.com/01org/cc-oci-runtime/blob/topic/documentation/documentation/docker-cc.png)

`cc-oci-runtime` creates a QEMU/KVM virtual machine for each container the Docker engine creates.

The container process is then spawned by [hyperstart](https://github.com/hyperhq/hyperstart/),
an agent running as a daemon on the guest operating system.
Hyperstart opens 2 virtio serial interfaces (Control and I/O) on the guest, and QEMU exposes them
as serial devices on the host. `cc-oci-runtime` uses the control device for sending container
management commands to hyperstart while the I/O serial device is used to pass I/O streams (`stdout`,
`stderr`, `stdin`) between the guest and the Docker Engine.

For any given container, both the init process and all potentially executed commands within that
container, together with their related I/O streams, need to go through 2 virtio serial interfaces
exported by QEMU. The [Clear Containers proxy (`cc-proxy`)](https://github.com/01org/cc-oci-runtime/tree/master/proxy)
multiplexes and demultiplexes those commands and streams for all container virtual machines.
There is only one `cc-proxy` instance running per Clear Containers host.

On the host, each container process is reaped by a Docker specific (`containerd-shim`) monitoring
daemon. As Clear Containers processes run inside their own virtual machines, `containerd-shim`
can not monitor, control or reap them. `cc-oci-runtime` fixes that issue by creating an
[additional shim process (`cc-shim`)](https://github.com/01org/cc-oci-runtime/tree/master/shim)
between `containerd-shim` and `cc-proxy`. A `cc-shim` instance will both forward signals and `stdin`
streams to the container process on the guest and pass the container `stdout` and `stderr` streams
back to the Docker engine via `containerd-shim`.
`cc-oci-runtime` creates a `cc-shim` daemon for each Docker container and for each command Docker
wants to run within an already running container (`docker exec`).

The container workload, i.e. the actual OCI bundle rootfs, is exported from the host to
the virtual machine via a 9pfs virtio mount point. Hyperstart uses this mount point as the root
filesystem for the container processes.

![Overall architecture](https://github.com/01org/cc-oci-runtime/blob/topic/documentation/documentation/overall-architecture.png)

#### Hypervisor

Clear Containers use [KVM](http://www.linux-kvm.org/page/Main_Page)/[QEMU](http://www.qemu-project.org/) to
create virtual machines where Docker containers will run:

![QEMU/KVM](https://github.com/01org/cc-oci-runtime/blob/topic/documentation/documentation/qemu.png)

Although Clear Containers can run with any recent QEMU release, containers boot time and memory
footprint are significantly optimized by using a specific QEMU version called [`qemu-lite`](https://github.com/01org/qemu-lite).

`qemu-lite` improvements comes through a new `pc-lite` machine type, mostly by:
- Removing many of the legacy hardware devices support so that the guest kernel does not waste
time initializing devices of no use for containers.
- Skipping the guest BIOS/firmware and jumping straight to the Clear Containers kernel.

#### Hyperstart

[`Hyperstart`](https://github.com/hyperhq/hyperstart) is a daemon running on the guest as an
agent for managing containers and processes potentially running within those containers.

It is statically built out of a compact C code base, with a strong focus on both simplicity
and memory footprint.
The `hyperstart` execution unit is the pod. A `hyperstart` pod is a container sandbox defined
by a set of namespaces (UTS, PID, mount and IPC). Although a pod can hold several containers,
`cc-oci-runtime` always runs a single container per pod.

`Hyperstart` sends and receives [specific commands](https://github.com/hyperhq/hyperstart/blob/master/src/api.h)
over a control serial interface for controlling and managing pods and containers. For example,
`cc-oci-runtime` will send the following `hyperstart` commands sequence when starting a container:

* `STARTPOD` creates a Pod sandbox and takes a `Pod` structure as its argument:
```Go
type Pod struct {
	Hostname              string                `json:"hostname"`
	DeprecatedContainers  []Container           `json:"containers,omitempty"`
	DeprecatedInterfaces  []NetworkInf          `json:"interfaces,omitempty"`
	Dns                   []string              `json:"dns,omitempty"`
	DeprecatedRoutes      []Route               `json:"routes,omitempty"`
	ShareDir              string                `json:"shareDir"`
	PortmappingWhiteLists *PortmappingWhiteList `json:"portmappingWhiteLists,omitempty"`
}
```
* `NEWCONTAINER` will create and start a container within the previously created
pod. This command takes a container description as its argument:

```Go
type Container struct {
	Id            string              `json:"id"`
	Rootfs        string              `json:"rootfs"`
	Fstype        string              `json:"fstype,omitempty"`
	Image         string              `json:"image"`
	Addr          string              `json:"addr,omitempty"`
	Volumes       []*VolumeDescriptor `json:"volumes,omitempty"`
	Fsmap         []*FsmapDescriptor  `json:"fsmap,omitempty"`
	Sysctl        map[string]string   `json:"sysctl,omitempty"`
	Process       *Process            `json:"process"`
	RestartPolicy string              `json:"restartPolicy"`
	Initialize    bool                `json:"initialize"`
	Ports         []Port              `json:"ports,omitempty"` //deprecated
}
```

`Hyperstart` uses a separate serial channel for passing the container processes output streams
(`stdout`, `stderr`) back to `cc-proxy` and receiving the input stream (`stdin`) for them.
As all streams for all containers are going through one single serial channel, hyperstart
prepends them with container specific sequence numbers. There are at most 2 sequence numbers
per container process, one for `stdout` and `stdin`, and another one for `stderr`.

#### Runtime

`cc-oci-runtime` is an OCI compatible container runtime and is responsible for handling all
commands specified by [the OCI runtime specification](https://github.com/opencontainers/runtime-spec)
and launching `cc-shim` instances.

Here we will describe how `cc-oci-runtime` handles the most important OCI commands.

##### `create`

When handling the OCI `create` command, `cc-oci-runtime` goes through the following steps:

1. Create the container namespaces (Only the network and mount namespaces are currently supported),
according to the container OCI configuration file.
2. Spawn the `cc-shim` process and have it wait on a couple of temporary pipes for:
   * A `cc-proxy` created file descriptor (one end of a socketpair) for the shim to connect to.
   * The container `hyperstart` sequence numbers for at most 2 I/O streams (One for `stdout` and `stdin`, one for `stderr`).
   `hyperstart` uses those sequence numbers to multiplex all streams for all processes through one serial interface (The
   virtio I/O serial one).
3. Run all the [OCI hooks](https://github.com/opencontainers/runtime-spec/blob/master/config.md#hooks) in the container namespaces,
as described by the OCI container configuration file.
4. [Set up the container networking](https://github.com/01org/cc-oci-runtime/blob/topic/documentation/documentation/architecture.md#networking).
This must happen after all hooks are done as one of them is potentially setting
the container networking namespace up.
5. Create the virtual machine running the container process. The VM `systemd` instance will spawn the `hyperstart` daemon.
6. Wait for `hyperstart` to signal that it is ready.
7. Send the pod creation command to `hyperstart`. The `hyperstart` pod is the container process sandbox.
8. Send the allocateIO command to the proxy, for getting the `hyperstart` I/O sequence numbers described in step 2.
9. Pass the `cc-proxy` socketpair file descriptor, and the I/O sequence numbers to the listening cc-shim process through the dedicated pipes.
10. The `cc-shim` instance is put into a stopped state to prevent it from doing I/O before the container is started.

![Docker create](https://github.com/01org/cc-oci-runtime/blob/topic/documentation/documentation/create.png)

At that point, the container sandbox is created in the virtual machine and `cc-shim` is stopped on the host.
However the container process itself is not yet running as one needs to call `docker start` to actually start it.

##### `start`

On namespace containers `start` launches a traditional Linux container process in its own set of namespaces.
With Clear Containers, the main task of `cc-oci-runtime` is to create and start a container within the
pod that got created during the `create` step. In practice, this means `cc-oci-runtime` follows
these steps:

1. `cc-oci-runtime` connects to `cc-proxy` and sends it the `attach` command to let it know which pod
we want to use to create and start the new container.
2. `cc-oci-runtime` sends a hyperstart `NEWCONTAINER` command to create and start a new container in
a given pod. The command is sent to `cc-proxy` who forwards it to the right hyperstart instance running
in the appropriate guest.
3. `cc-oci-runtime` resumes `cc-shim` so that it can now connect to the `cc-proxy` and acts as
a signal and I/O streams proxy between `containerd-shim` and `cc-proxy`.

##### `exec`

`docker exec` allows one to run an additional command within an already running container.
With Clear Containers, this translates into sending a `EXECCMD` command to hyperstart so
that it runs a command into a running container belonging to a certain pod.
All I/O streams from the executed command will be passed back to Docker through a newly
created `cc-shim`.

The `exec` code path is partly similar to the `create` one and `cc-oci-runtime` goes through
the following steps:

1. `cc-oci-runtime` connects to `cc-proxy` and sends it the `attach` command to let it know which pod
we want to use to run the `exec` command.
2. `cc-oci-runtime` sends the allocateIO command to the proxy, for getting the `hyperstart` I/O sequence
numbers for the `exec` command I/O streams.
3. `cc-oci-runtime` sends an hyperstart `EXECMD` command to start the command in the right container
The command is sent to `cc-proxy` who forwards it to the right hyperstart instance running
in the appropriate guest.
4. Spawn the `cc-shim` process for it to forward the output streams (stderr and stdout) and the `exec`
command exit code to Docker.

Now the `exec`'ed process is running in the virtual machine, sharing the UTS, PID, mount and IPC
namespaces with the container's init process.

##### `kill`

When sending the OCI `kill` command, container runtimes should send a [UNIX signal](https://en.wikipedia.org/wiki/Unix_signal)
to the container process.
In the Clear Containers context, this means `cc-oci-runtime` needs a way to send a signal
to the container process within the virtual machine. As `cc-shim` is responsible for
forwarding signals to its associated running containers, `cc-oci-runtime` naturally
calls `kill` on the `cc-shim` PID.

However, `cc-shim` is not able to trap `SIGKILL` and `SIGSTOP` and thus `cc-oci-runtime`
needs to follow a different code path for those 2 signals.
Instead of `kill`'ing the `cc-shim` PID, it will go through the following steps:

1. `cc-oci-runtime` connects to `cc-proxy` and sends it the `attach` command to let it know
on which pod the container it is trying to `kill` is running.
2. `cc-oci-runtime` sends an hyperstart `KILLCONTAINER` command to `kill` the container running
on the guest. The command is sent to `cc-proxy` who forwards it to the right hyperstart instance
running in the appropriate guest.

##### `delete`

`docker delete` is about deleting all resources held by a stopped/killed container. Running
containers can not be `delete`d unless the OCI runtime is explictly being asked to. In that
case it will first `kill` the container and only then `delete` it.

The resources held by a Clear Container are quite different from the ones held by a host
namespace container e.g. run by `runc`. `cc-oci-runtime` needs mostly to delete the pod
holding the stopped container on the virtual machine, shut the hypervisor down and finally
delete all related proxy resources:

1. `cc-oci-runtime` connects to `cc-proxy` and sends it the `attach` command to let it know
on which pod the container it is trying to to `delete` is running.
2. `cc-oci-runtime` sends an hyperstart `DESTROYPOD` command to `destroy` the pod holding the
container running on the guest. The command is sent to `cc-proxy` who forwards it to the right
hyperstart instance running in the appropriate guest.
3. After deleting the last running pod, `hyperstart` will gracefully shut the virtual machine
down.
4. `cc-oci-runtime` sends the `BYE` command to `cc-proxy`, to let it know that a given virtual
machine is shut down. `cc-proxy` will then clean all its internal resources associated with this
VM.

#### Proxy

`cc-proxy` is a daemon offering access to the [`hyperstart`](https://github.com/hyperhq/hyperstart)
VM agent to multiple `cc-shim` and `cc-oci-runtime` clients.
Only a single instance of `cc-proxy` per host is necessary as it can be used for several different VMs.
Its main role is to:
- Arbitrate access to the `hyperstart` control channel between all the `cc-oci-runtime` instances and the `cc-shim` ones.
- Route the I/O streams between the various `cc-shim` instances and `hyperstart`.

`cc-proxy` provides 2 client interfaces:

- A UNIX, named socket for all `cc-oci-runtime` instances on the host to send commands to `cc-proxy`.
- One socket pair per `cc-shim` instance, to send stdin and receive stdout and stderr I/O streams. See the
[cc-shim section](https://github.com/01org/cc-oci-runtime/blob/topic/documentation/documentation/architecture.md#shim)
for more details about that interface.

The protocol on the `cc-proxy` UNIX named socket supports the following commands:

- `Hello`: This command is for `cc-oci-runtime` to let `cc-proxy` know about a newly created VM that will
hold containers. This command payload contains the `hyperstart` control and I/O UNIX socket paths created
and exported by QEMU, and `cc-proxy` will connect to both of them after receiving the `Hello` command.
- `Bye`: This is the opposite of `Hello`, i.e. `cc-oci-runtime` uses this command to let `cc-proxy` know
that it can release all resources related to the VM described in the command payload.
- `Attach`: `cc-oci-runtime` uses that command as a VM multiplexer as it allows it to notify `cc-proxy` about
which VM it wants to talk to. In other words, this commands allows `cc-oci-runtime` to attach itself to a
running VM.
- `AllocateIO`: As `hyperstart` can potentially handle I/O streams from multiple container processes at the
same time, it needs to be able to associate any given stream to a container process. This is done by `hyperstart`
allocating a set of at most 2 so-called sequence numbers per container process. `cc-oci-runtime` will send
the `AllocateIO` command to `cc-proxy` to have it request `hyperstart` to allocate those sequence numbers.
They will be passed as command line arguments to `cc-shim`, who will then use them to e.g. prepend its stdin
stream packets with the right sequence number.
- `Hyper`: This command is used by both `cc-oci-runtime` and `cc-shim` to forward `hyperstart` specific
commands.

For more details about `cc-proxy`'s protocol, theory of operations or debugging tips, please read
[`cc-proxy` README](https://github.com/01org/cc-oci-runtime/tree/master/proxy).

#### Shim

Docker's `containerd-shim` is designed around the assumption that it can monitor and reap the actual
container process. As `containerd-shim` runs on the host, it can not directly monitor a process running
within a virtual machine. At most it can see the QEMU process, but that is not enough.
With Clear Containers, `cc-shim` acts as the container process that `containerd-shim` can monitor. Therefore
`cc-shim` needs to handle all container I/O streams (`stdout`, `stdin` and `stderr`) and forward all signals
`containerd-shim` decides to send to the container process.

`cc-shim` has an implicit knowledge about which VM agent will handle those streams and signals and thus act as
an encapsulation layer between `containerd-shim` and `hyperstart`:

- It fragments and encapsulates the standard input stream from containerd-shim into `hyperstart` stream packets:
```
  ┌───────────────────────────┬───────────────┬────────────────────┐
  │ IO stream sequence number │ Packet length │ IO stream fragment │
  │         (8 bytes)         │    (4 bytes)  │                    │
  └───────────────────────────┴───────────────┴────────────────────┘
```
- It de-encapsulates and assembles standard output and error `hyperstart` stream packets into an output stream
that it forwards to `containerd-shim`
- It translates all UNIX signals (except `SIGKILL` and `SIGSTOP`) into `hyperstart` `KILLCONTAINER` commands
that it sends to the VM via `cc-proxy` UNIX named socket.

The IO stream sequence numbers are passed from `cc-runtime` to `cc-shim` when the former spawns the latter.
They are generated by `hyperstart` and `cc-oci-runtime` fetches them by sending the `AllocateIO` command to
`cc-proxy`.

As an example, let's say that running the `pwd` command from a container standard input will generate
`/tmp` from the container standard output. `hyperstart` assigned this specific process 8888 and 8889 respectively
as the stdin, stdout and stderr sequence numbers.
With `cc-shim` and Clear Containers, this example would look like:

![cc-shim](https://github.com/01org/cc-oci-runtime/blob/topic/documentation/documentation/shim.png)

#### Networking

Containers will typically live in their own, possibly shared, networking namespace.
At some point in a container lifecycle, container engines will set up that namespace
to add the container to a network which is isolated from the host network, but which is shared between containers

In order to do so, container engines will usually add one end of a `virtual ethernet
(veth)` pair into the container networking namespace. The other end of the `veth` pair
is added to the container network.

This is a very namespace-centric approach as QEMU can not handle `veth` interfaces.
Instead it typically creates `TAP` interfaces for adding connectivity to a virtual
machine.

To overcome that incompatibility between typical container engines expectations
and virtual machines, `cc-oci-runtime` networking transparently bridges `veth`
interfaces with `TAP` ones:

![Clear Containers networking](https://github.com/01org/cc-oci-runtime/blob/topic/documentation/documentation/network.png)

The [virtcontainers library](https://github.com/containers/virtcontainers#cnm) has some more
details on how `cc-oci-runtime` implements [CNM](https://github.com/docker/libnetwork/blob/master/docs/design.md).