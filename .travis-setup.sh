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

glib_version=2.49.4
json_glib_version=1.2.2
check_version=0.10.0

# Install required dependencies to build
# glib, json-glib, check and cc-oci-runtime
sudo apt-get -qq install valgrind lcov uuid-dev pkg-config \
  zlib1g-dev libffi-dev gettext libpcre3-dev texinfo gtk-doc-tools cppcheck

mkdir cor-dependencies
pushd cor-dependencies

# Build glib
curl -L -O "https://github.com/GNOME/glib/archive/${glib_version}.tar.gz"
tar -xvf "${glib_version}.tar.gz"
pushd "glib-${glib_version}"
bash autogen.sh
make
sudo make install
popd

# Build json-glib
curl -L -O "https://github.com/GNOME/json-glib/archive/${json_glib_version}.tar.gz"
tar -xvf "${json_glib_version}.tar.gz"
pushd "json-glib-${json_glib_version}"
bash autogen.sh
make
sudo make install
popd

# Build check
curl -L -O "https://github.com/libcheck/check/archive/${check_version}.tar.gz"
tar -xvf "${check_version}.tar.gz"
pushd "check-${check_version}"
autoreconf --install
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
