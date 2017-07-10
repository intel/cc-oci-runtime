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

# Description: This script installs Clear Containers on a
#   CentOS 7 or RHEL 7 system.
#

SCRIPT_PATH=$(dirname "$(readlink -f "$0")")
source "${SCRIPT_PATH}/installation-setup.sh"
source "${SCRIPT_PATH}/../versions.txt"

# List of packages to install to satisfy build dependencies
pkgs=""

mnl_dev_pkg="libmnl-devel"

# general
pkgs+=" zlib-devel"
pkgs+=" gettext-devel"
pkgs+=" libtool-ltdl-devel"
pkgs+=" libtool-ltdl"
pkgs+=" glib2-devel"
pkgs+=" bzip2"
pkgs+=" m4"

# for yum-config-manager
pkgs+=" yum-utils"

# runtime dependencies
pkgs+=" libuuid-devel"
pkgs+=" libmnl"
pkgs+=" ${mnl_dev_pkg}"
pkgs+=" libffi-devel"
pkgs+=" pcre-devel"

# qemu lite dependencies
pkgs+=" libattr-devel"
pkgs+=" libcap-devel"
pkgs+=" libcap-ng-devel"
pkgs+=" pixman-devel"

pkgs+=" gcc-c++"

if [ "$os_distribution" = rhel ]
then
    # RHEL doesn't provide "*-devel" packages unless the "Optional RPMS"
    # repository is enabled. However, to make life fun, there isn't a
    # clean way to determine if that repository is enabled (the output
    # format of "yum repolist" seems to change frequently and
    # subscription-manager's output isn't designed for easy script
    # consumption).
    #
    # Therefore, the safest approach seems to be to check if a known
    # required development package is known to yum(8). If it isn't, the
    # user will need to enable the extra repository.
    #
    # Note that this issue is unique to RHEL: yum on CentOS provides
    # access to developemnt packages by default.

    yum info "${mnl_dev_pkg}" >/dev/null 2>&1
    if [ $? -ne 0 ]
    then
        echo >&2 "ERROR: You must enable the 'optional' repository for '*-devel' packages"
        exit 1
    fi
fi

if [ "$os_distribution" = rhel ]
then
    cc_repo_url="http://download.opensuse.org/repositories/home:/clearlinux:/preview:/clear-containers-2.1/RHEL_7/home:clearlinux:preview:clear-containers-2.1.repo"
elif [ "$os_distribution" = centos ]
then
    cc_repo_url="http://download.opensuse.org/repositories/home:/clearlinux:/preview:/clear-containers-2.1/CentOS_7/home:clearlinux:preview:clear-containers-2.1.repo"
else
    echo >&2 "ERROR: Unrecognised distribution: $os_distribution"
    echo >&2 "ERROR: This script is designed to work on CentOS and RHEL systems only."
    exit 1
fi

sudo yum -y update
eval sudo yum -y install "$pkgs"

sudo yum groupinstall -y 'Development Tools'

pushd "$deps_dir"

# Install pre-requisites for gcc
curl -L -O ftp://gcc.gnu.org/pub/gcc/infrastructure/gmp-${gmp_version}.tar.bz2
compile gmp gmp-${gmp_version}.tar.bz2 gmp-${gmp_version}

curl -L -O ftp://gcc.gnu.org/pub/gcc/infrastructure/mpfr-${mpfr_version}.tar.bz2
compile mpfr mpfr-${mpfr_version}.tar.bz2 mpfr-${mpfr_version}

curl -L -O ftp://gcc.gnu.org/pub/gcc/infrastructure/mpc-${mpc_version}.tar.gz
compile mpc mpc-${mpc_version}.tar.gz mpc-${mpc_version}

# Install go
go_setup

# Install glib
glib_setup

# Install json-glib
json_glib_setup

# Install gcc
gcc_setup

# Install qemu-lite
qemu_lite_setup

popd

# Install docker
sudo mkdir -p /etc/yum.repos.d/
sudo tee /etc/yum.repos.d/docker.repo <<EOF
[dockerrepo]
name=Docker Repository
baseurl=https://yum.dockerproject.org/repo/main/centos/7/
enabled=1
gpgcheck=1
gpgkey=https://yum.dockerproject.org/gpg
EOF
sudo yum install -y \
    docker-engine-1.12.1-1.el7.centos.x86_64 \
    docker-engine-selinux-1.12.1-1.el7.centos.noarch

# Install kernel and CC image
sudo yum-config-manager --add-repo "${cc_repo_url}"
sudo yum -y install linux-container clear-containers-image

# Configure cc-oci-runtime
export PATH=$PATH:"${prefix_dir}"/go/bin
cor=github.com/01org/cc-oci-runtime
# Currently it is the latest version for cc-oci-runtime
commit=tags/2.1.9
release=${commit##*/}
go get "$cor"
pushd "$GOPATH/src/$cor"
git checkout -b "$release" "$commit"
# autoconf-archive package does not exist in RHEL we need to download all m4
# files required
source "${SCRIPT_PATH}/curl-autoconf-archive.sh"
./autogen.sh \
    --prefix="${prefix_dir}" \
    --disable-tests \
    --disable-functional-tests \
    --enable-autogopath \
    --with-cc-image=/usr/share/clear-containers/clear-containers.img \
    --with-cc-kernel=/usr/share/clear-containers/vmlinux.container \
    --with-qemu-path="${prefix_dir}"/bin/qemu-system-x86_64
make -j5 && sudo make install
popd

# Configure CC by default
sudo mkdir -p /etc/systemd/system/docker.service.d/
cat <<EOF|sudo tee /etc/systemd/system/docker.service.d/clr-containers.conf
[Service]
ExecStart=
ExecStart=/usr/bin/dockerd -D --add-runtime cor=${prefix_dir}/bin/cc-oci-runtime --default-runtime=cor
EOF

sudo systemctl daemon-reload
sudo systemctl restart docker
sudo systemctl start cc-proxy.socket
