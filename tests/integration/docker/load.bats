#!/usr/bin/env bats
# *-*- Mode: sh; sh-basic-offset: 8; indent-tabs-mode: nil -*-*

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

#Based on docker commands

SRC="${BATS_TEST_DIRNAME}/../../lib/"

setup() {
	source $SRC/test-common.bash
	runtime_docker
}

@test "Load container" {
	container=$(random_name)
	$DOCKER_EXE run -itd --name $container busybox
	$DOCKER_EXE commit $container mynewimage
	$DOCKER_EXE save mynewimage> /tmp/mynewimage.tar
	$DOCKER_EXE load < /tmp/mynewimage.tar
	$DOCKER_EXE images | grep "mynewimage"
	$DOCKER_EXE rmi mynewimage
	$DOCKER_EXE rm -f $container
}
