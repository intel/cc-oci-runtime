# Install the Clear Containers runtime on Centos 7


## Introduction

Installing Clear Containers on CentOS is slightly more difficult than on other distributions mainly due to the fact that it does not provide current versions of gcc and glibc.

Hence, this guide explains how to install updated packages in `/usr/local`, to avoid conflicting with the base packages.

## Warning

Note carefully the comment in the section [Install container kernel and rootfs](#install-container-kernel-and-rootfs). You MUST update the packages listed regularly to ensure your Clear Containers benefit from performance improvements, new features and most importantly CVE security fixes.

## Install updated glib and json-glib

Note: these packages need to be built with the default CentOS compiler since they don't build by default with gcc 6.

```
$ export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
$ sudo yum groupinstall 'Development Tools'
$ sudo yum install zlib-devel libffi-devel

$ curl -L -O "https://download.gnome.org/sources/glib/2.46/glib-2.46.2.tar.xz"
$ tar -xvf glib-2.46.2.tar.xz
$ pushd glib-2.46.2
$ ./configure --prefix=/usr/local --disable-silent-rules && make -j5 && sudo make install
$ popd

$ curl -L -O "https://download.gnome.org/sources/json-glib/1.2/json-glib-1.2.2.tar.xz"
$ tar -xvf json-glib-1.2.2.tar.xz
$ pushd json-glib-1.2.2
$ ./configure --prefix=/usr/local --disable-silent-rules && make -j5 && sudo make install
$ popd

```

## Install updated gcc

This is required for `qemu-lite`.

```
$ sudo yum install gmp-devel mpfr-devel libmpc-devel gcc-c++
$ curl -L -O http://www.mirrorservice.org/sites/ftp.gnu.org/gnu/gcc/gcc-6.2.0/gcc-6.2.0.tar.gz
$ tar xvf gcc-6.2.0.tar.gz
$ pushd gcc-6.2.0
$ ./configure --enable-languages=c --disable-multilib --prefix=/usr/local/gcc-6.2.0
$ make -j5 && sudo make install
$ popd
```

## Install qemu-lite

Although this package installs the `qemu-lite` version of `qemu`, the binary will still be called `qemu-system-x86_64`.
Requires at least gcc 5.

```
$ export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
$ export PATH=/usr/local/gcc-6.2.0/bin:$PATH
$ sudo yum install libcap-ng-devel pixman-devel libattr-devel libcap-devel
$ curl -L -O https://github.com/01org/qemu-lite/archive/3f889234fcd31ac4119aae57559b1286e09e1568.tar.gz
$ tar xvf 3f889234fcd31ac4119aae57559b1286e09e1568.tar.gz
$ pushd qemu-lite-3f889234fcd31ac4119aae57559b1286e09e1568
$ CC=/usr/local/gcc-6.2.0/bin/gcc ./configure \
				--disable-bluez 	--disable-brlapi \
				--disable-bzip2 	--disable-curl \
				--disable-curses 	--disable-debug-tcg \
				--disable-fdt 		--disable-glusterfs \
				--disable-gtk 		--disable-libiscsi \
				--disable-libnfs 	--disable-libssh2 \
				--disable-libusb 	--disable-linux-aio \
				--disable-lzo 		--disable-opengl \
				--disable-rbd 		--disable-qom-cast-debug \
				--disable-rdma 		--disable-sdl \
				--disable-seccomp 	--disable-slirp \
				--disable-snappy 	--disable-spice \
				--disable-strip	 	--disable-tcg-interpreter \
				--disable-tcmalloc	--disable-tools \
				--disable-tpm 		--disable-usb-redir \
				--disable-uuid 		--disable-vnc \
				--disable-vte 		--disable-vnc-{jpeg,png,sasl} \
				--disable-xen 		--enable-attr \
				--enable-cap-ng 	--enable-kvm \
				--enable-virtfs 	--target-list=x86_64-softmmu \
				--extra-cflags="-fno-semantic-interposition -O3 -falign-functions=32" \
				--datadir=/usr/local/share/qemu-lite \
				--libdir=/usr/local/lib64/qemu-lite \
				--libexecdir=/usr/local/libexec/qemu-lite
$ make -j5 && sudo make install
$ popd
```

## Install container kernel and rootfs

- Point your browser at the URL below and download the latest versions of `linux-container` and `clear-containers-image` packages:
	- http://download.opensuse.org/repositories/home:/clearlinux:/preview:/clear-containers-2.1/CentOS_7/x86_64/
- Install the packages:

	 ```
	 $ sudo rpm -ivh linux-container-*.x86_64.rpm clear-containers-image-*.x86_64.rpm
	 ```

**WARNING:** You should repeat this step regularly to ensure your Clear Containers are running with the latest versions of these packages.


## Install latest Golang

cc-oci-runtime supports golang 1.7 and 1.8
The next instructions were taken from [golang site](https://golang.org/doc/install)

```
$ curl -L -O https://storage.googleapis.com/golang/go1.8.linux-amd64.tar.gz
$ sudo tar -C /usr/local -xzf go1.8.linux-amd64.tar.gz
$ export PATH=$PATH:/usr/local/go/bin
$ mkdir ~/go
$ export GOPATH=~/go

```

## Install the clear containers runtime from sources

```
$ export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
$ export PATH=/usr/local/gcc-6.2.0/bin:$PATH
$ sudo yum install libuuid-devel pcre-devel libffi-devel zlib-devel gettext-devel pkgconfig libmnl-devel
$ go get github.com/01org/cc-oci-runtime
$ pushd go/src/github.com/01org/cc-oci-runtime
$ git checkout tags/2.1.0
$ ./autogen.sh \
	--prefix=/usr/local \
	--disable-tests \
	--disable-functional-tests \
	--with-cc-image=/usr/share/clear-containers/clear-containers.img \
	--with-cc-kernel=/usr/share/clear-containers/vmlinux.container \
	--with-qemu-path=/usr/local/bin/qemu-system-x86_64
$ make -j5 && sudo make install
$ popd

```

## Install docker 1.12

```
$ sudo tee /etc/yum.repos.d/docker.repo <<EOF
[dockerrepo]
name=Docker Repository
baseurl=https://yum.dockerproject.org/repo/main/centos/7/
enabled=1
gpgcheck=1
gpgkey=https://yum.dockerproject.org/gpg
EOF

$ sudo yum install docker-engine-1.12.1-1.el7.centos.x86_64 docker-engine-selinux-1.12.1-1.el7.centos.noarch

```

For more information on docker installation, see:
- https://docs.docker.com/engine/installation/linux/centos/


## Configure docker to use Clear Containers by default

```
$ sudo mkdir -p /etc/systemd/system/docker.service.d/
$ sudo vi /etc/systemd/system/docker.service.d/clr-containers.conf
[Service]
ExecStart=
ExecStart=/usr/bin/dockerd -D --add-runtime cor=/usr/local/bin/cc-oci-runtime --default-runtime=cor
```

## Restart the docker systemd service

```
$ sudo systemctl daemon-reload
$ sudo systemctl restart docker
```

## Start cc-proxy socket

```
$ sudo systemctl start cc-proxy.socket
```

## Run a Clear Container
You are now ready to run clear containers. For example:

```
$ sudo docker run -ti fedora bash
```

## Known Issues

Currently `cc-oci-runtime v2.1` only works correctly with `docker-engine=1.12.1-1.el7.centos.x86_64`. While we update `cc-oci-runtime` to work with later versions of docker, we recommend running the command below to stop the `docker-engine` package from being updated when new package versions become available. Note that there is a risk in doing this since newer versions of Docker may contain security fixes.

```
$ sudo echo "exclude=docker-engine" >> /etc/yum.repos.d/docker.repo
```

## Notes
You can override the default runtime docker will use by specifying `--runtime $runtime` (`$runtime` can be either `cor` or `runc` (dockers default)). For example, to specify explicitly that you wish to use the Clear Containers runtime:

```
$ sudo docker run --runtime cor -ti ubuntu bash
```
