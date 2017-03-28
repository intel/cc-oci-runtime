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

# Based on docker commands

SRC="${BATS_TEST_DIRNAME}/../../lib/"
exit_status=55

setup() {
	source $SRC/test-common.bash
	runtime_docker
	kill_processes_before_start
}

teardown() {
	check_no_processes_up
}

@test "Exit Code from container process when running non-interactive" {
	container=$(random_name)
	run $DOCKER_EXE run --name $container ubuntu /usr/bin/perl -e "exit $exit_status"
	[ "${status}" -eq "$exit_status" ]
	$DOCKER_EXE rm -f $container
}

@test "Exit Code from container process when running interactive" {
	container=$(random_name)
	run $DOCKER_EXE run --name $container -ti ubuntu /usr/bin/perl -e "exit $exit_status"
	[ "${status}" -eq "$exit_status" ]
	$DOCKER_EXE rm -f $container
}

@test "Verifying exit code with docker ps" {
	container=$(random_name)
	run $DOCKER_EXE run --name $container -ti ubuntu bash -c "exit $exit_status"
	[ "${status}" -eq "$exit_status" ]
	run $($DOCKER_EXE ps -a | grep $container | grep "Exited ($exit_status)")
	$DOCKER_EXE rm -f $container
}
