# Install the Intel® Clear Containers runtime on Centos 7.0

## Required Setup

The installation requires the current user to run sudo without specifying a password. Verify this with the following commands:

```
$ su -
# echo "$some_user ALL=(ALL:ALL) NOPASSWD: ALL" | (EDITOR="tee -a" visudo)
$ exit

```

## Installation steps

1. Ensure the system packages are up-to-date with the command:

```
$ sudo yum -y update

```
2. Install Git:

```
sudo yum install -y git

```
3. Create the installation directory and clone the repository with the following commands:

```
$ mkdir -p $HOME/go/src/github/01org
$ cd $HOME/go/src/github/01org
$ git clone https://github.com/01org/cc-oci-runtime.git
$ cd cc-oci-runtime

```
4. Run the installation script rhel-setup.sh:

```
$ ./installation/rhel-setup.sh

```

## Verify the installation was successful

1. Check the `cc-oci-runtime` version with the following command:

```
$ cc-oci-runtime --version

```

2. Run an example with the following command:

```
$ sudo docker run -ti fedora bash

```
