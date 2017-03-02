#!/usr/bin/env bats
# *-*- Mode: sh; sh-basic-offset: 8; indent-tabs-mode: nil -*-*

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

@test "Run as non-root user" {
	container=$(random_name)
	run $DOCKER_EXE run --name $container -u postgres postgres whoami
	[ "${status}" -eq 0 ]
	[[ "${output}" == 'postgres'* ]]
	$DOCKER_EXE rm -f $container
}

@test "Groups for non-root user" {
	container=$(random_name)
	run $DOCKER_EXE run --name $container -u postgres postgres groups
	[ "${status}" -eq 0 ]
	[[ "${output}" == 'postgres ssl-cert'* ]]
	$DOCKER_EXE rm -f $container
}

@test "Run as root user" {
	container=$(random_name)
	run $DOCKER_EXE run --name $container -u root:root postgres whoami
	[ "${status}" -eq 0 ]
	[[ "${output}" == 'root'* ]]
	$DOCKER_EXE rm -f $container
}

@test "Run with additional groups" {
	run $DOCKER_EXE run --rm --group-add audio --group-add nogroup busybox id
	[ "${status}" -eq 0 ]
	echo "${output}" | grep "uid=0(root) gid=0(root) groups=[wheel10,()]*29(audio),99(nogroup)"
}

@test "Run with non-existing numeric gid" {
	skip "This is not working. Hyperstart issue: https://github.com/hyperhq/hyperstart/issues/250"
	# When passed a non-existent numeric id, hyperstart errors out saying that the
	# user does not exist. This works with runc.
	# Docker mentions "When passing a numeric ID, the user does not have
	# to exist in the container."
	# This needs to be fixed in hyperstart codebase.
	# Passing a non-existent group name on the command line is not permitted however
	# and docker itself handles the error.
	run $DOCKER_EXE run --rm --group-add audio --group-add nogroup --group-add 777 busybox id
	[ "${status}" -eq 0 ]
	echo "${output}" | grep "777"
}
