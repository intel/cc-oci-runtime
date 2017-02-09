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
	kill_processes_before_start
}

teardown() {
	check_no_processes_up
}

@test "Create a container" {
	container=$(random_name)
	$DOCKER_EXE create -ti --name $container busybox true
	$DOCKER_EXE ps -a | grep $container
	$DOCKER_EXE rm -f $container
}

@test "Create network" {
	$DOCKER_EXE network ls
	$DOCKER_EXE network create -d bridge my-bridge-network
	$DOCKER_EXE network ls | grep "my-bridge-network"
	$DOCKER_EXE network rm my-bridge-network
}
