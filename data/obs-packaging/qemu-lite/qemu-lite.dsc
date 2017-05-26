Format: 3.0 (quilt)
Source: qemu-lite
Version: 2.7.1
Section: devel
Priority: optional
Maintainer: clearlinux.org team <dev@lists.clearlinux.org>
Build-Depends: debhelper (>= 9), cpio, libelf-dev, rsync, libdw-dev, pkg-config, flex, bison, libaudit-dev, bc, python-dev, gawk, autoconf, automake, libtool, libltdl-dev, libglib2.0-dev, libglib2.0-0, libcap-dev, libcap-ng-dev, libattr1-dev, m4, libnuma-dev, zlib1g-dev, libpixman-1-0, libpixman-1-dev
Standards-Version: 3.9.6
Homepage: https://clearlinux.org/features/clear-containers
DEBTRANSFORM-RELEASE: 1

Package: qemu-lite
Architecture: amd64
Depends: ${shlibs:Depends}, ${misc:Depends}, ${perl:Depends},
Description: QEMU is a generic and open source machine & userspace emulator and
virtualizer.
