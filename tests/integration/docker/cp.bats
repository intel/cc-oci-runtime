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

@test "Check mounted files at /etc after a docker cp" {
	container=$(random_name)
	content="test"
	testfile=$(mktemp --tmpdir="$BATS_TMPDIR" --suffix=-cor-test)
	$DOCKER_EXE run --name $container -tid ubuntu bash
	driver=$($DOCKER_EXE inspect --format '{{ .GraphDriver.Name }}' $container)
	if [ "$driver" == "devicemapper" ] ; then
		$DOCKER_EXE rm -f $container
		skip
	fi
	echo $content > $testfile
	$DOCKER_EXE cp $testfile $container:/root/
	$DOCKER_EXE exec -i $container bash -c "ls /root/$(basename $testfile)"
	$DOCKER_EXE exec -i $container bash -c "[ -s /etc/resolv.conf ]"
	rm -f $testfile
	$DOCKER_EXE rm -f $container
}

teardown() {
	check_no_processes_up
}
