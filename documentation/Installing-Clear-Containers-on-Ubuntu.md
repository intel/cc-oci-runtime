# Installing  Clear Containers runtime on Ubuntu

## Introduction

Clear Containers 2.1 is available for Ubuntu for versions **16.04**, **16.10**
and **17.04**.
Clear Containers supports the latest version of Docker CE (currently 17.05), with
the exception of Swarm. If you want to use Swarm, you must install Docker
version 1.12.1.

## Install the supported version of Docker
This step is optional and required only in the case where docker is not installed
on the system, or the explicitly supported version of docker is required.

Follow one of the two options below:

### Option 1) Docker 1.12.1 (for compatibility with Swarm)

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
sudo apt-mark hold docker-engine
```

### Option 2) Docker 17

**Warning:** Clear Containers 2.1 and Swarm will not work correctly on Docker 17.

```
sudo apt-get install \
	apt-transport-https \
	ca-certificates \
	curl \
	software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo add-apt-repository \
	"deb [arch=amd64] https://download.docker.com/linux/ubuntu \
	$(lsb_release -cs) \
	stable"
sudo apt-get update
sudo apt-get install docker-ce
```

For more information on installing Docker please refer to the
[Docker Guide](https://docs.docker.com/engine/installation/linux/ubuntu)

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
