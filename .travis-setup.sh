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

cwd=$(cd `dirname "$0"`; pwd -P)
source $cwd/versions.txt

#
# Install go
#

go_tarball="go${go_version}.linux-amd64.tar.gz"
curl -L -O "https://storage.googleapis.com/golang/$go_tarball"
tar xvf $go_tarball 1>/dev/null
# Unfortunately, go doesn't support vendoring outside of GOPATH (maybe in 1.8?)
# So, we setup a GOPATH tree with our vendored dependencies.
# See: https://github.com/golang/go/issues/14566
mkdir -p "$GOPATH/src"
cp -r vendor/* "$GOPATH/src"
# We also need to put the runtime into its right place in the GOPATH so we can
# self-import internal packages
mkdir -p "$GOPATH/src/github.com/01org/"
ln -s $PWD "$GOPATH/src/github.com/01org/"

#
# Install cc-oci-runtime dependencies
#

# Ensure "make install" as root can find clang
#
# See: https://github.com/travis-ci/travis-ci/issues/2607
export CC=$(which "$CC")

gnome_dl=https://download.gnome.org/sources

# Install required dependencies to build
# glib, json-glib, libmnl-dev check and cc-oci-runtime
sudo apt-get -qq install valgrind lcov uuid-dev pkg-config \
  zlib1g-dev libffi-dev gettext libpcre3-dev cppcheck \
  libmnl-dev

function compile {
	name="$1"
	tarball="$2"
	directory="$3"

	fail=0
	{
		tar -xvf ${tarball}
		pushd ${directory}
		./configure --disable-silent-rules || fail=1
		make -j5 || fail=1
		sudo make install || fail=1
		popd
	} >>$name-build.log 2>&1

	if [ $fail -eq 1 ]; then
		cat $name-build.log
	fi
}

mkdir cor-dependencies
pushd cor-dependencies

# Build glib
glib_major=`echo $glib_version | cut -d. -f1`
glib_minor=`echo $glib_version | cut -d. -f2`
curl -L -O "$gnome_dl/glib/${glib_major}.${glib_minor}/glib-${glib_version}.tar.xz"
compile glib glib-${glib_version}.tar.xz glib-${glib_version}

# Build json-glib
json_major=`echo $json_glib_version | cut -d. -f1`
json_minor=`echo $json_glib_version | cut -d. -f2`
curl -L -O "$gnome_dl/json-glib/${json_major}.${json_minor}/json-glib-${json_glib_version}.tar.xz"
compile json-glib json-glib-${json_glib_version}.tar.xz json-glib-${json_glib_version}

# Build check
# We need to build check as the check version in the OS used by travis isn't
# -pedantic safe.
curl -L -O "https://github.com/libcheck/check/releases/download/${check_version}/check-${check_version}.tar.gz"
compile check check-${check_version}.tar.gz check-${check_version}

# Install bats
git clone https://github.com/sstephenson/bats.git
pushd bats
sudo ./install.sh /usr/local
popd

popd
