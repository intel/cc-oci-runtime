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

@test "modifying a container with exec" {
	container=$(random_name)
	$DOCKER_EXE run --name $container -d ubuntu bash -c "sleep 50"
	$DOCKER_EXE ps -a | grep "sleep 50"
	$DOCKER_EXE exec -d $container bash -c "echo 'hello world' > file"
	$DOCKER_EXE exec $container bash -c "cat /file"
	$DOCKER_EXE rm -f $container
}

@test "exec a container with privileges" {
	container=$(random_name)
	$DOCKER_EXE run -d --name $container ubuntu bash -c "sleep 30"
	$DOCKER_EXE exec -i --privileged $container bash -c "mount -t tmpfs none /mnt"
	$DOCKER_EXE exec $container bash -c "df -h | grep "/mnt""
	$DOCKER_EXE rm -f $container
}

@test "copying file from host to container using exec" {
	container=$(random_name)
	content="hello world"
	$DOCKER_EXE run --name $container -d ubuntu bash -c "sleep 30"
	echo $content | $DOCKER_EXE exec -i $container bash -c "cat > /home/file.txt"
	$DOCKER_EXE exec -i $container bash -c "cat /home/file.txt" | grep "$content"
	$DOCKER_EXE rm -f $container
}

@test "stdout forwarded using exec" {
	container=$(random_name)
	$DOCKER_EXE run --name $container -d ubuntu bash -c "sleep 30"
	$DOCKER_EXE exec -ti $container ls /etc/resolv.conf 2>/dev/null | grep "/etc/resolv.conf" 
	$DOCKER_EXE rm -f $container
}

@test "stderr forwarded using exec" {
	container=$(random_name)
	$DOCKER_EXE run --name $container -d ubuntu bash -c "sleep 30"
	if $DOCKER_EXE exec $container ls /etc/foo >/dev/null; then
		false;
	else
		true;
	fi
	$DOCKER_EXE rm -f $container
}

@test "check exit code using exec" {
	container=$(random_name)
	$DOCKER_EXE run --name $container -d ubuntu bash -c "sleep 30"
	run $DOCKER_EXE exec $container bash -c "exit 42"
	[ "$status" -eq 42 ]
	$DOCKER_EXE rm -f $container
}
