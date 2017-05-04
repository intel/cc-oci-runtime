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

# Description: This script downloads and installs the autoconf-archive macros
#   necessary to configure this project. This is invoked for distro builds where
#   the necessary package is not available to install, and is also useful to
#   hand run under certain circumstances.
#   Should be run from the top level directory
#

# autoconf-archive url
autoconf_archive_url="http://git.savannah.gnu.org/gitweb/?p=autoconf-archive.git;a=blob_plain;f=m4"
mkdir -p m4/
# curl the required autoconf archive files into the correct place
curl -L "${autoconf_archive_url}/ax_code_coverage.m4" -o m4/ax_code_coverage.m4
curl -L "${autoconf_archive_url}/ax_valgrind_check.m4" -o m4/ax_valgrind_check.m4
