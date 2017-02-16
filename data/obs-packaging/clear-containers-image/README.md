#clear-containers image

##Debian:

To build debian package you need to download the linux kernel that is pointed at
``Source0:`` at clear-containers-image.spec

Then create tarball:

``clear-containers-image_{version}.orig.tar.xz``

``tar -cJf clear-containers-image_{version}.orig.tar.xz clear-{version}-containers.img.xz LICENSE``

