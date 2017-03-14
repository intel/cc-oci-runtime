# Installing  Clear Containers runtime on Ubuntu

## Introduction

Clear Containers 2.1 is available for Ubuntu for versions **16.04** and **16.10**.
Clear containers explicitly supports Docker 1.12.1. Later versions of Docker may be
used but some issues may exist.


## Install the supported version of Docker
This step is optional and required only in the case where docker is not installed
on the system, or the explicitly supported version of docker is required.

The `add-apt-repository` and `apt-get` commands below reference
`ubuntu-xenial`. Both Ubuntu 16.04 and 16.10 require this reference
since the version of Docker supported by Clear Containers is not
available as a Ubuntu 16.10 package.

```
sudo groupadd docker
sudo -E gpasswd -a $USER docker
sudo apt-get install -y apt-transport-https ca-certificates
curl -fsSL https://yum.dockerproject.org/gpg | sudo apt-key add -
sudo add-apt-repository "deb https://apt.dockerproject.org/repo/ ubuntu-xenial main"
sudo apt-get update
sudo apt-get install -y --allow-downgrades docker-engine=1.12.1-0~xenial
sudo sudo apt-mark hold docker-engine
```

## Setup the repository for Clear Container

```
sudo sh -c "echo 'deb http://download.opensuse.org/repositories/home:/clearlinux:/preview:/clear-containers-2.1/xUbuntu_$(lsb_release -rs)/ /' >> /etc/apt/sources.list.d/cc-oci-runtime.list"
curl -fsSL http://download.opensuse.org/repositories/home:clearlinux:preview:clear-containers-2.1/xUbuntu_$(lsb_release -rs)/Release.key | sudo apt-key add -
```

## Install Clear Containers 2.1
```
sudo apt-get update
sudo apt-get install -y cc-oci-runtime
```


## Configure docker to use Clear Containers by default
```
sudo mkdir -p /etc/systemd/system/docker.service.d/
cat << EOF | sudo tee /etc/systemd/system/docker.service.d/clr-containers.conf
[Service]
ExecStart=
ExecStart=/usr/bin/dockerd -D --add-runtime cor=/usr/bin/cc-oci-runtime --default-runtime=cor
EOF
```

## Restart the docker and Clear Containers systemd services

```
sudo systemctl daemon-reload
sudo systemctl restart docker
sudo systemctl enable cc-proxy.socket
sudo systemctl start cc-proxy.socket
```

## Run a Clear Container
You are now ready to run Clear Containers. For example:

```
sudo docker run -ti fedora bash
```
