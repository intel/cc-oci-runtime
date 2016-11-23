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

nested=$(cat /sys/module/kvm_intel/parameters/nested 2>/dev/null \
    || echo N)

echo "INFO: Nested kvm available: $nested"

if [ -n "$SEMAPHORE_CACHE_DIR" ]
then
    # Running under SemaphoreCI
    prefix_dir="$SEMAPHORE_CACHE_DIR/cor"
else
    prefix_dir="$HOME/.cache/cor"
fi

deps_dir="${prefix_dir}/dependencies"
mkdir -p "$deps_dir"

export LD_LIBRARY_PATH="${prefix_dir}/lib:$LD_LIBRARY_PATH"
export PKG_CONFIG_PATH="${prefix_dir}/lib/pkgconfig:$PKG_CONFIG_PATH"
export ACLOCAL_FLAGS="-I \"${prefix_dir}/share/aclocal\" $ACLOCAL_FLAG"
export GOROOT=$HOME/go
export GOPATH=$HOME/gopath
export PATH=$GOROOT/bin:$GOPATH/bin:$PATH
export PATH="${prefix_dir}/bin:${prefix_dir}/sbin:$PATH"
