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

glib_version=2.46.2
json_glib_version=1.2.2
check_version=0.10.0

gnome_dl=https://download.gnome.org/sources

# Install required dependencies to build
# glib, json-glib, check and cc-oci-runtime
sudo apt-get -qq install valgrind lcov uuid-dev pkg-config \
  zlib1g-dev libffi-dev gettext libpcre3-dev texinfo gtk-doc-tools cppcheck

mkdir cor-dependencies
pushd cor-dependencies

# Build glib
glib_major=`echo $glib_version | cut -d. -f1`
glib_minor=`echo $glib_version | cut -d. -f2`
curl -L -O "$gnome_dl/glib/${glib_major}.${glib_minor}/glib-${glib_version}.tar.xz"
tar -xvf "glib-${glib_version}.tar.xz"
pushd "glib-${glib_version}"
./configure
make
sudo make install
popd

# Build json-glib
json_major=`echo $json_glib_version | cut -d. -f1`
json_minor=`echo $json_glib_version | cut -d. -f2`
curl -L -O "$gnome_dl/json-glib/${json_major}.${json_minor}/json-glib-${json_glib_version}.tar.xz"
tar -xvf "json-glib-${json_glib_version}.tar.xz"
pushd "json-glib-${json_glib_version}"
./configure
make
sudo make install
popd

# Build check
curl -L -O "https://github.com/libcheck/check/releases/download/${check_version}/check-${check_version}.tar.gz"
tar -xvf "check-${check_version}.tar.gz"
pushd "check-${check_version}"
./configure
make
sudo make install
popd

# Install bats
git clone https://github.com/sstephenson/bats.git
pushd bats
sudo ./install.sh /usr/local
popd

popd
