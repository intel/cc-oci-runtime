# Install the Clear Containers runtime on Fedora

## Introduction

Clear Containers 2.1 is available for Fedora\* versions **24** and **25**.
Clear Containers supports the latest version of Docker\* CE, currently 17.05,
with the exception of Swarm\*. If you require Swarm, install Docker version
1.12.1.


## Install the Clear Containers runtime
```
$ sudo -E dnf config-manager --add-repo \
http://download.opensuse.org/repositories/home:clearlinux:preview:clear-containers-2.1/Fedora\_25/home:clearlinux:preview:clear-containers-2.1.repo
$ sudo dnf install cc-oci-runtime linux-container
```

## Install Docker

This step is optional and only required in case Docker is not
installed on the system or an specific version of Docker is
required.

Execute the commands for only one of the two following options.

### Option 1) Docker 1.12.1 compatible with Swarm

Docker 1.12.1 is not available in Fedora 25. Therefore, pull from the Fedora 24 repository:

```
$ sudo dnf config-manager --add-repo \
	https://yum.dockerproject.org/repo/main/fedora/24
```
Append gpg key information to end of docker's fedora 24 repo config:
```
$ sudo vi /etc/yum.repos.d/yum.dockerproject.org_repo_main_fedora_24.repo
gpgcheck=1
gpgkey=https://yum.dockerproject.org/gpg
```
Install docker engine:
```
$ sudo dnf install docker-engine-1.12.1-1.fc24
```

### Option 2) Docker 17

**Caution:** Clear Containers 2.1 and Swarm will not work correctly on Docker 17.

```
$ sudo dnf -y install dnf-plugins-core
$ sudo dnf config-manager \
    --add-repo \
    https://download.docker.com/linux/fedora/docker-ce.repo
$ sudo dnf makecache fast
$ sudo dnf install docker-ce
```

For more information on installing Docker please refer to the
[Docker Guide](https://docs.docker.com/engine/installation/linux/fedora)

## Configure docker to use Clear Containers by default
```
$ sudo mkdir -p /etc/systemd/system/docker.service.d/
$ sudo vi /etc/systemd/system/docker.service.d/clr-containers.conf
[Service]
ExecStart=
ExecStart=/usr/bin/dockerd -D --add-runtime cor=/usr/bin/cc-oci-runtime --default-runtime=cor
```
## Restart the docker systemd service
```
$ sudo systemctl daemon-reload
$ sudo systemctl restart docker
```

## Run a Clear Container
You are now ready to run Clear Containers. For example
```
$ sudo docker run -ti fedora bash
```
## Notes
You can override the default runtime docker will use by specifying
`--runtime $runtime` (`$runtime` can be either `cor` or `runc` (dockers
default)). For example, to specify explicitly that you wish to use the
Clear Containers runtime:
```
$ sudo docker run --runtime cor -ti ubuntu bash
```
Note: this should not be necessary if you update `--default-runtime` setting in `clr-containers.conf` as described above.

## Increase limits (optional)
If you are running a recent enough kernel (4.3+), you should consider
increasing the `TasksMax=` systemd setting. Without this, the number of
Clear Containers you are able to run will be limited.  A example of the error
which may occur can be seen at:
[issue 154](https://github.com/01org/cc-oci-runtime/issues/154).


Run the commands below and if they display `OK`, proceed, else skip this
section
```
$ grep -q ^pids /proc/cgroups && echo OK
```
Assuming you got an `OK`
```
$ cd /etc/systemd/system/docker.service.d/
$ sudo vi /etc/systemd/system/docker.service.d/clr-containers.conf
[Service] # Allow maximum number of containers to run.
TasksMax=infinity
```
