# Install the Clear Containers runtime on Clear Linux

## Introduction

Clear Containers 2.1 is available in Clear Linux since version **13310**.
Run `swupd` command to ensure the host is using the latest Clear Linux Version.

```
$ sudo swupd update
```

## Install the Clear Containers bundle

```
$ sudo swupd bundle-add containers-basic
```

## Restart the docker systemd service

Docker on Clear Linux provides 2 service files to start the daemon, and only one will be enabled.
- docker.service (runc)
- docker-cor.service (cc-oci-runtime)

If you are running Clear Linux on baremetal or on a VM with Nested Virtualization activated the service `docker-cor` will be launched and default runtime will be `cc-oci-runtime`. If you are running Clear Linux on a VM without Nested Virtualization the service `docker` will be launched and default runtime will be `runc`.

```
$ sudo systemctl daemon-reload
$ sudo systemctl restart docker-cor
```

## Run a Clear Container
You are now ready to run Clear Containers. For example:

```
$ sudo docker run -ti fedora bash
```
