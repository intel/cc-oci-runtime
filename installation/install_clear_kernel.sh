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


set -x
set -e

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 CLEAR_RELEASE KERNEL_VERSION PATH"
    echo "       Install the clear kernel image KERNEL_VERSION from clear CLEAR_RELEASE in PATH."
    exit 1
fi

clear_release="$1"
kernel_version="$2"
install_path="$3"
clear_install_path="/usr/share/clear-containers"
kernel_raw=vmlinux-${kernel_version}.container
kernel_zip=vmlinuz-${kernel_version}.container
cc_kernel_raw_link_name="vmlinux.container"
cc_kernel_zip_link_name="vmlinuz.container"
tmpdir=$(mktemp -d -t $(basename $0).XXXXXXXXXXX) || exit 1
pushd $tmpdir

echo -e "Install clear containers kernel ${kernel_version}"

curl -LO "https://download.clearlinux.org/releases/${clear_release}/clear/x86_64/os/Packages/linux-container-${kernel_version}.x86_64.rpm"
rpm2cpio linux-container-${kernel_version}.x86_64.rpm | cpio -ivdm
sudo install -D --owner root --group root --mode 0700 .${clear_install_path}/${kernel_raw} ${install_path}/${kernel_raw}
sudo install -D --owner root --group root --mode 0700 .${clear_install_path}/${kernel_zip} ${install_path}/${kernel_zip}

echo -e "Create symbolic link ${install_path}/${cc_kernel_raw_link_name}"
sudo ln -fs ${install_path}/${kernel_raw} ${install_path}/${cc_kernel_raw_link_name}
echo -e "Create symbolic link ${install_path}/${cc_kernel_zip_link_name}"
sudo ln -fs ${install_path}/${kernel_zip} ${install_path}/${cc_kernel_zip_link_name}

# cleanup
rm -f linux-container-${kernel_version}.x86_64.rpm
# be careful here, we don't want to rm something silly, note the leading .
rm -r .${clear_install_path}
rmdir ./usr/share
rmdir ./usr
popd
