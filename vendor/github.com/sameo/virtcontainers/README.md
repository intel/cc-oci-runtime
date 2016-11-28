[![Build Status](https://travis-ci.org/sameo/virtcontainers.svg?branch=master)](https://travis-ci.org/sameo/virtcontainers)
[![Go Report Card](https://goreportcard.com/badge/github.com/sameo/virtcontainers)](https://goreportcard.com/report/github.com/sameo/virtcontainers)
[![Coverage Status](https://coveralls.io/repos/github/sameo/virtcontainers/badge.svg?branch=sameo%2Ftopic%2Funit)](https://coveralls.io/github/sameo/virtcontainers)
[![GoDoc](https://godoc.org/github.com/sameo/virtcontainers?status.svg)](https://godoc.org/github.com/sameo/virtcontainers)

# VirtContainers

VirtContainers is a Go package for building hardware virtualized container runtimes.

## Scope

VirtContainers is not a container runtime implementation, but aims at factorizing
hardware virtualization code in order to build VM based container runtimes.

The few existing VM based container runtimes (Clear Containers, RunV, Rkt
kvm stage 1) all share the same hardware virtualization semantics but use different
code bases to implement them. VirtContainers goal is to factorize this code into
a common Go library.

Ideally VM based container runtime implementations would become translation layers
from the runtime specification they implement to the VirtContainers API.

## Out of scope

Implementing yet another container runtime is out of VirtContainers scope. Any tools
or executables provided with VirtContainers are only provided for demonstration or
testing purposes.

### VirtContainers and CRI

VirtContainers API is inspired by Kubernetes
[CRI](https://github.com/kubernetes/kubernetes/blob/master/docs/proposals/container-runtime-interface-v1.md)
runtime API because we believe it provides the right level of abstractions
for containerized pods.
Here we want to emphasize that despite the API similarities between those 2
projects, VirtContainers primary goal is not to build a Kubernetes CRI runtime server
but to provide a generic, runtime specification agnostic, hardware virtualized
containers library.

## Design

### Goals

VirtContainers is a container specification agnostic Go package and thus tries to
abstract the various container runtime specifications (OCI, AppC and CRI) and present
that as its high level API.

### Pods

The VirtContainers execution unit is a Pod, i.e. VirtContainers callers start pods
where containers will be running.

Virtcontainers creates a pod by starting a virtual machine and setting the pod up within
that environment. Starting a pod means launching all containers with the VM pod runtime
environment.

### Hypervisors

The virtcontainers package relies on hypervisors to start and stop virtual machine where
pods will be running. An hypervisor is defined by an Hypervisor interface implementation,
and the default implementation is the QEMU one.

### Agents

During the lifecycle of a container, the runtime running on the host needs to interact with
the virtual machine guest OS in order to start new commands to be executed as part of a given
container workload, set new networking routes or interfaces, fetch a container standard or
error output, and so on.
There are many existing and potential solutions to resolve that problem and virtcontainers abstract
this through the Agent interface.

## API

The high level VirtContainers API is the following one:

### Pod API

* `CreatePod(podConfig PodConfig)` creates a Pod.
The Pod is prepared and will run into a virtual machine. It is not started, i.e. the VM is not running after `CreatePod()` is called.

* `DeletePod(podID string)` deletes a Pod.
The function will fail if the Pod is running. In that case `StopPod()` needs to be called first.

* `StartPod(podID string)` starts an already created Pod.

* `StopPod(podID string)` stops an already running Pod.

* `ListPod()` lists all running Pods on the host.

* `EnterPod(cmd Cmd)` enters a Pod root filesystem and runs a given command.

* `PodStatus(podID string)` returns a detailed Pod status.

### Container API

* `CreateContainer(podID string, container ContainerConfig)` creates a Container on a given Pod.

* `DeleteContainer(podID, containerID string)` deletes a Container from a Pod. If the container is running it needs to be stopped first.

* `StartContainer(podID, containerID string)` starts an already created container.

* `StopContainer(podID, containerID string)` stops an already running container.

* `EnterContainer(podID, containerID string, cmd Cmd)` enters an already running container and runs a given command.

* `ContainerStatus(podID, containerID string)` returns a detailed container status.

## Virtc example

Here we explain how to use the pod API from __virtc__ command line.

### Prepare your environment

#### Get your kernel

_Fedora_
```
$ sudo -E dnf config-manager --add-repo http://download.opensuse.org/repositories/home:clearlinux:preview:clear-containers-2.0/Fedora_24/home:clearlinux:preview:clear-containers-2.0.repo
$ sudo dnf install linux-container 
```

_Ubuntu_
```
$ sudo sh -c "echo 'deb http://download.opensuse.org/repositories/home:/clearlinux:/preview:/clear-containers-2.0/xUbuntu_16.04/ /' >> /etc/apt/sources.list.d/cc-oci-runtime.list"
$ sudo apt install linux-container
```

#### Get your image

It has to be a recent Clear Linux image to make sure it contains hyperstart binary.
You can dowload the following tested [image](https://download.clearlinux.org/releases/11130/clear/clear-11130-containers.img.xz), or any version more recent.

```
$ wget https://download.clearlinux.org/releases/11130/clear/clear-11130-containers.img.xz
$ unxz clear-11130-containers.img.xz
$ sudo cp clear-11130-containers.img /usr/share/clear-containers/clear-containers.img
```

#### Get virtc

_Download virtc_
```
$ go get github.com/sameo/virtcontainers
```

_Build virtc_
```
$ cd $GOPATH/src/github.com/sameo/virtcontainers
$ cd virtc && go build
```

### Run virtc

All following commands needs to be run as root. Currently, __virtc__ only starts single container pods.

_Create your container bundle_

As an example we will create a busybox bundle:

```
$ mkdir -p /tmp/bundles/busybox/
$ docker pull busybox
$ cd /tmp/bundles/busybox/
$ mkdir rootfs
$ docker export $(docker create busybox) | tar -C rootfs -xvf -
$ echo -e '#!/bin/sh\ncd "\"\n"sh"' > rootfs/.containerexec
$ echo -e 'HOME=/root\nPATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin\nTERM=xterm' > rootfs/.containerenv
```

#### Run a new pod (Create + Start)
```
./virtc pod run --bundle="/tmp/bundles/busybox/rootfs" --agent="hyperstart" --init-cmd="/bin/sh"
```
#### Create a new pod
```
./virtc pod create --bundle="/tmp/bundles/busybox/rootfs" --agent="hyperstart" --init-cmd="/bin/sh"
```
This should generate that kind of output
```
Created pod 306ecdcf-0a6f-4a06-a03e-86a7b868ffc8
```

#### Start an existing pod
```
./virtc pod start --id=306ecdcf-0a6f-4a06-a03e-86a7b868ffc8
```

#### Stop an existing pod
```
./virtc pod stop --id=306ecdcf-0a6f-4a06-a03e-86a7b868ffc8
```

#### Get the status of an existing pod and its containers
```
./virtc pod status --id=306ecdcf-0a6f-4a06-a03e-86a7b868ffc8
```
This should generate the following output:
```
POD ID                                  STATE   HYPERVISOR      AGENT
306ecdcf-0a6f-4a06-a03e-86a7b868ffc8    running qemu            hyperstart

CONTAINER ID    STATE
1               ready
```

#### Delete an existing pod
```
./virtc pod delete --id=306ecdcf-0a6f-4a06-a03e-86a7b868ffc8
```

#### List all existing pods
```
./virtc pod list
```
This should generate that kind of output
```
POD ID                                  STATE   HYPERVISOR      AGENT
306ecdcf-0a6f-4a06-a03e-86a7b868ffc8    running qemu            hyperstart
92d73f74-4514-4a0d-81df-db1cc4c59100    running qemu            hyperstart
7088148c-049b-4be7-b1be-89b3ae3c551c    ready   qemu            hyperstart
6d57654e-4804-4a91-b72d-b5fe375ed3e1    ready   qemu            hyperstart
```
