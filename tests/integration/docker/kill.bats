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
cc_shim="/usr/libexec/cc-shim"

setup() {
	source $SRC/test-common.bash
	runtime_docker
	kill_processes_before_start
}

@test "Kill a container" {
	container=$(random_name)
	$DOCKER_EXE run -d -ti --name $container busybox sh
	$DOCKER_EXE kill $container
	$DOCKER_EXE rm $container
}

@test "Kill container with SIGUSR1" {
	container=$(random_name)
	exit_code=15
	attempts=5
	signal=SIGUSR1
	$DOCKER_EXE run -dti --name $container ubuntu bash -c "trap \"exit ${exit_code}\" $signal ; while : ; do sleep 1; done"
	$DOCKER_EXE kill -s $signal $container
	# waiting kill signal
	for i in `seq 1 $attempts` ; do
		if [ "$($DOCKER_EXE ps -a | grep $container | grep Exited)" != "" ]; then
			break
		fi
		sleep 1
	done
	# check exit code
	$DOCKER_EXE ps -a | grep $container | grep "Exited (${exit_code})"
	$DOCKER_EXE rm $container
}

teardown() {
	check_no_processes_up
}
