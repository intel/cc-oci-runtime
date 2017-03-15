# Install the Clear Containers runtime on Fedora 25

## Install the Clear Containers runtime
```
$ sudo -E dnf config-manager --add-repo \
http://download.opensuse.org/repositories/home:clearlinux:preview:clear-containers-2.1/Fedora\_25/home:clearlinux:preview:clear-containers-2.1.repo
$ sudo dnf install cc-oci-runtime linux-container
```

## Install docker 1.12

For compatibility reasons, docker 1.12.1 is required. This is not available
in Fedora 25, so we will pull from the Fedora 24 repository:

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
Clear Containers you are able to run will be limited.  A sample of the error which may manifest can be seen at [issue 154] (https://github.com/01org/cc-oci-runtime/issues/154).


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
