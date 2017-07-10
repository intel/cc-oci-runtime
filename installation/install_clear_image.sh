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

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 CLEAR_RELEASE PATH"
    echo "       Install the clear rootfs image from clear CLEAR_RELEASE in PATH."
    exit 1
fi

clear_release="$1"
install_path="$2"
image=clear-${clear_release}-containers.img
cc_img_link_name="clear-containers.img"
base_url="https://download.clearlinux.org/releases/${clear_release}/clear"
tmpdir=$(mktemp -d -t $(basename $0).XXXXXXXXXXX) || exit 1
pushd $tmpdir

echo "Download clear containers image"
curl -LO "${base_url}/${image}.xz"

echo "Validate clear containers image checksum"
curl -LO "${base_url}/${image}.xz-SHA512SUMS"
sha512sum -c ${image}.xz-SHA512SUMS

echo "Extract clear containers image"
unxz ${image}.xz

sudo mkdir -p ${install_path}
echo "Install clear containers image"
sudo install -D --owner root --group root --mode 0755 ${image} ${install_path}/${image}

echo -e "Create symbolic link ${install_path}/${cc_img_link_name}"
sudo ln -fs ${install_path}/${image} ${install_path}/${cc_img_link_name}

# clean up
rm -f ${image} ${image}.xz-SHA512SUMS
popd
