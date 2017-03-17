
# Clear Containers Quick Start Development and Test environment

## Introduction

Clear Containers is normally run on bare metal machines with Intel CPU's
supporting Intel VT-x. However Clear Containers can also be run inside a virtual
machine provided the hypervisor supports VT-x nesting. KVM supports VT-x nesting
today.

Installation of Clear Containers on bare metal will result in changes to the host system
and to the configuration of Docker on the host.

In order to provide a seamless evaluation, development and test environment without
the need for any modification to the host system, we provide tools to
create, launch and manage a fully configured *Ubuntu virtual machine* where you can try
Clear Containers.

This virtual machine is also setup with all the dependencies to build and modify the
Clear Container(s) runtime.

This setup is particularly useful when your want try to modify networking functionality,
or when trying out docker swarm or kubernetes.


## ciao-down

*ciao-down* is a small utility for setting up a VM that contains everything you need to run
Clear Containers.

All you need to do to use *ciao-down* on your machine is:

* Ensure go 1.7 or greater is installed on the host system.
* Ensure that kvm nested virtualization is enabled on your host


   ```
   $ cat /sys/module/kvm_intel/parameters/nested
   Y
   ```

  If not, enable nested virtualization as per your distribution specific instuctions.
  Nested virtualization is enabled by default on Ubuntu.
  For fedora you can follow the instructions here https://fedoraproject.org/wiki/How_to_enable_nested_virtualization_in_KVM



* Install *ciao-down*:

   ```
   $ go get github.com/01org/ciao/testutil/ciao-down
   ```

* Create a Clear Containers VM:


   ```
   $ $GOPATH/bin/ciao-down prepare -vmtype clearcontainers
   $ $GOPATH/bin/ciao-down connect
   ```

## Usage of ciao-down:


```
$ ciao-down [prepare|start|stop|quit|status|connect|delete]

- prepare : creates a new VM for ciao or clear containers
- start : boots a stopped VM
- stop : cleanly powers down a running VM
- quit : quits a running VM
- status : prints status information about the ciao-down VM
- connect : connects to the VM via SSH
- delete : shuts down and deletes the VM
```

For more details https://github.com/01org/ciao/tree/master/testutil/ciao-down
