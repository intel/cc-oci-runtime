#!/bin/bash

#  This file is part of cc-oci-runtime.
#
#  Copyright (C) 2017 Intel Corporation
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

SCRIPT_PATH=$(dirname "$(readlink -f "$0")")
source "${SCRIPT_PATH}/installation-setup.sh"
source "${SCRIPT_PATH}/../versions.txt"

# Install required dependencies to build
# glib, json-glib, libmnl-dev, check, gcc, cc-oci-runtime and qemu-lite

pkgs=""

# general
pkgs+=" zlib-devel"
pkgs+=" gettext-devel"
pkgs+=" pkgconfig"
pkgs+=" libtool-ltdl-devel"
pkgs+=" libtool-ltdl"
pkgs+=" glib2-devel"

# runtime dependencies
pkgs+=" libuuid-devel"
pkgs+=" libmnl"
pkgs+=" libmnl-dev"
pkgs+=" libffi-devel"
pkgs+=" pcre-devel"

# qemu lite dependencies
pkgs+=" libattr-devel"
pkgs+=" libcap-devel"
pkgs+=" libcap-ng-devel"
pkgs+=" pixman-devel"

# gcc
pkgs+=" gmp-devel"
pkgs+=" mpfr-devel"
pkgs+=" libmpc-devel"
pkgs+=" gcc-c++"

sudo yum -y update
eval sudo yum -y install "$pkgs"

# Install pre-requisites for gcc
curl -L -O ftp://gcc.gnu.org/pub/gcc/infrastructure/mpfr-${mpfr_version}.tar.bz2
compile mpfr mpfr-${mpfr_version}.tar.bz2 mpfr-${mpfr_version}

curl -L -O ftp://gcc.gnu.org/pub/gcc/infrastructure/gmp-${gmp_version}.tar.bz2
compile gmp gmp-${gmp_version}.tar.bz2 gmp-${gmp_version}

curl -L -O ftp://gcc.gnu.org/pub/gcc/infrastructure/mpc-${mpc_version}.tar.gz
compile mpc mpc-${mpc_version}.tar.gz mpc-${mpc_version}

# Install go
go_setup

# Install glib
glib_setup

# Install json-glib
json-glib_setup

# Install gcc
gcc_setup

# Install qemu-lite
qemu-lite_setup

# Install kernel and CC image
sudo yum-config-manager --add-repo http://download.opensuse.org/repositories/home:/clearlinux:/preview:/clear-containers-2.1/RHEL_7/home:clearlinux:preview:clear-containers-2.1.repo
sudo yum -y update
sudo yum -y install linux-container clear-containers-image clear-containers-selinux

# Configure cc-oci-runtime
export PATH=$PATH:"${prefix_dir}"/go/bin
cor=github.com/01org/cc-oci-runtime
# Currently it is the latest version for cc-oci-runtime
commit=tags/2.1.0
go get "$cor"
pushd "$GOPATH/src/$cor"
git checkout "$commit"
./autogen.sh \
    --prefix="${prefix_dir}" \
    --disable-tests \
    --disable-functional-tests \
    --with-cc-image=/usr/share/clear-containers/clear-containers.img \
    --with-cc-kernel=/usr/share/clear-containers/vmlinux.container \
    --with-qemu-path="${prefix_dir}"/bin/qemu-system-x86_64
make -j5 && sudo make install
popd

# Configure CC by default
mkdir -p /etc/systemd/system/docker.service.d/
cat > /etc/systemd/system/docker.service.d/clr-containers.conf << EOF
[Service]
ExecStart=
ExecStart=/usr/bin/dockerd -D --add-runtime cor=/usr/local/bin/cc-oci-runtime --default-runtime=cor
EOF
