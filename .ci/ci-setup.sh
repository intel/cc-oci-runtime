#!/bin/bash
#  This file is part of cc-oci-runtime.
#
#  Copyright (C) 2016 Intel Corporation
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

set -e -x

root=$(cd `dirname "$0"`/..; pwd -P)
source "$root/versions.txt"

if [ "$SEMAPHORE" = true ]
then
    # SemaphoreCI has different environments that builds can run in. The
    # default environment does not have docker enabled so it is
    # necessary to specify a docker-enabled build environment on the
    # semaphoreci.com web site.
    #
    # However, currently, the docker-enabled environment does not
    # provide nested KVM (whereas the default environment does), so
    # manually enable nested kvm for the time being.
    sudo rmmod kvm-intel || :
    sudo sh -c "echo 'options kvm-intel nested=y' >> /etc/modprobe.d/dist.conf" || :
    sudo modprobe kvm-intel || :
fi

source $(dirname "$0")/ci-common.sh
source "$root/installation/installation-setup.sh"

if [ "$nested" = "Y" ]
then
    # Ensure the user can access the kvm device
    sudo chmod g+rw /dev/kvm
    sudo chgrp "$USER" /dev/kvm
fi

# Install go
go_setup

#
# Install cc-oci-runtime dependencies
#

# Ensure "make install" as root can find clang
#
# See: https://github.com/travis-ci/travis-ci/issues/2607
export CC=$(which "$CC")

gnome_dl=https://download.gnome.org/sources

# Install required dependencies to build
# glib, json-glib, libmnl-dev, check, gcc, cc-oci-runtime and qemu-lite

pkgs=""

# general
pkgs+=" pkg-config"
pkgs+=" gettext"
pkgs+=" rpm2cpio"
pkgs+=" valgrind"

# runtime dependencies
pkgs+=" uuid-dev"
pkgs+=" cppcheck"
pkgs+=" libmnl-dev"
pkgs+=" libffi-dev"
pkgs+=" libpcre3-dev"

# runtime + qemu-lite
pkgs+=" zlib1g-dev"

# qemu-lite
pkgs+=" libpixman-1-dev"

# gcc
pkgs+=" libcap-ng-dev"
pkgs+=" libgmp-dev"
pkgs+=" libmpfr-dev"
pkgs+=" libmpc-dev"

# code coverage
pkgs+=" lcov"

# chronic(1)
pkgs+=" moreutils"

# qemu-lite won't be built
# some unit tests need qemu-system
if [ "$nested" != "Y" ]
then
	pkgs+=" qemu-system-x86"
fi

sudo apt-get -qq update
eval sudo apt-get -qq install "$pkgs"

pushd "$deps_dir"

# Build glib
glib_setup

# Build json-glib
json-glib_setup

# Build check
# We need to build check as the check version in the OS used by travis isn't
# -pedantic safe.
if ! lib_installed "check" "${check_version}"
then
    file="check-${check_version}.tar.gz"

    if [ ! -e "$file" ]
    then
        curl -L -O "https://github.com/libcheck/check/releases/download/${check_version}/$file"
    fi

    compile check check-${check_version}.tar.gz check-${check_version}
fi

# Install bats
bats_setup

if [ "$nested" != "Y" ]
then
    popd
    exit 0
fi

# Build gcc
gcc_setup

# Build qemu-lite
qemu-lite_setup

# install kernel + Clear Containers image
mkdir -p assets
pushd assets
clr_dl_site="https://download.clearlinux.org"
clr_release=$(curl -L "${clr_dl_site}/latest")
clr_kernel_base_url="${clr_dl_site}/releases/${clr_release}/clear/x86_64/os/Packages"

sudo mkdir -p "$clr_assets_dir"

# find newest containers kernel
clr_kernel=$(curl -l -s -L "${clr_kernel_base_url}" |\
    grep -o "linux-container-[0-9][0-9.-]*\.x86_64.rpm" |\
    sort -u)

# download kernel
if [ ! -e "${clr_assets_dir}/${clr_kernel}" ]
then
    if [ ! -e "$clr_kernel" ]
    then
        curl -L -O "${clr_kernel_base_url}/${clr_kernel}"
    fi

    # install kernel
    # (note: cpio on trusty does not support "-D")
    rpm2cpio "${clr_kernel}"| (cd / && sudo cpio -idv)
fi

clr_image_url="${clr_dl_site}/current/clear-${clr_release}-containers.img.xz"
clr_image_compressed=$(basename "$clr_image_url")

# uncompressed image name
clr_image=${clr_image_compressed/.xz/}

# download image
if [ ! -e "${clr_assets_dir}/${clr_image}" ]
then
    for file in "${clr_image_url}-SHA512SUMS" "${clr_image_url}"
    do
        [ ! -e "$file" ] && curl -L -O "$file"
    done

    # verify image
    checksum_file="${clr_image_compressed}-SHA512SUMS"
    sha512sum -c "${checksum_file}"

    # unpack image
    unxz --force "${clr_image_compressed}"

    # install image
    sudo install "${clr_image}" "${clr_assets_dir}"

    rm -f "${checksum_file}" "${clr_image}" "${clr_image_compressed}"
fi

# change kernel+image ownership
sudo chown -R "$USER" "${clr_assets_dir}"

# create image symlink (kernel will already have one)
clr_image_link=clear-containers.img
sudo rm -f "${clr_assets_dir}/${clr_image_link}"
(cd "${clr_assets_dir}" && sudo ln -s "${clr_image}" "${clr_image_link}")

popd

popd

if [ "$SEMAPHORE" = true ]
then
    distro=$(lsb_release -c|awk '{print $2}' || :)
    if [ "$distro" = trusty ]
    then
        # Configure docker to use the runtime
        docker_opts=""
        docker_opts+=" --add-runtime cor=cc-oci-runtime"
        docker_opts+=" --default-runtime=cor"

        sudo initctl stop docker

        # Remove first as apt-get doesn't like downgrading this package
        # on trusty.
        sudo apt-get -qq purge docker-engine

        sudo apt-get -qq install \
            docker-engine="$docker_engine_semaphoreci_ubuntu_version"

        echo "DOCKER_OPTS=\"$docker_opts\"" |\
            sudo tee -a /etc/default/docker

        sudo initctl restart docker

    else
        echo "ERROR: unhandled Semaphore distro: $distro"
        exit 1
    fi
fi
