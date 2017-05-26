# Install the Clear Containers runtime on Clear Linux

## Introduction

Clear Containers 2.1 is available in Clear Linux since version **13310**.
Run `swupd` command to ensure the host is using the latest Clear Linux Version.

```
$ sudo swupd update
```

## Install the Clear Containers bundle

```
$ sudo swupd bundle-add containers-virt
```

## Restart the docker systemd service

Docker on Clear Linux provides a `docker.service` service file to start the `docker` daemon.
The daemon will use `runc` or `cc-oci-runtime` depending on the environment:


If you are running Clear Linux on baremetal or on a VM with Nested Virtualization activated, `docker` will use `cc-oci-runtime` as the default runtime. If you are running Clear Linux on a VM without Nested Virtualization,  `docker` will use `runc` as the default runtime. It is not necessary to configure Docker to use `cc-oci-runtime` manually since docker itself will automatically use this runtime on systems that support it.

*Note: to check which runtime your system is using, run:*
```
sudo docker info | grep Runtime
```

Restart `docker` service:
```
$ sudo systemctl restart docker
```

## Run a Clear Container
You are now ready to run Clear Containers. For example:

```
$ sudo docker run -ti fedora bash
```
