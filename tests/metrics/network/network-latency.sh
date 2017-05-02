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
#
# Description:
# This will measure latency when we do a ping
# from one container to another (docker <-> docker)

set -e

SCRIPT_PATH=$(dirname "$(readlink -f "$0")")

source "${SCRIPT_PATH}/../../lib/test-common.bash"

# Image name (ping installed by default)
image=busybox
# Number of packets (sent)
number=10

function setup {
	runtime_docker
	kill_processes_before_start
}

# This script will perform all the measurements using a local setup

# Test latency docker<->docker using ping

function latency {
	setup
	latency_result=$(mktemp)
	container_name="first_container"
	second_container_name="second_container"
	$DOCKER_EXE run -tid --name ${container_name} ${image} sh > /dev/null
	ip_address=$($DOCKER_EXE inspect --format "{{.NetworkSettings.IPAddress}}" ${container_name})
	$DOCKER_EXE run -ti --rm --name ${second_container_name} ${image} sh -c "ping -c ${number} ${ip_address}" > "$latency_result"
	latency_average=$(cat $latency_result | grep avg | tail -1 | awk '{print $4}' | cut -d '/' -f 2)
	$DOCKER_EXE rm -f ${container_name} > /dev/null
	rm -f "$latency_result"
}

latency
echo "The average latency is : $latency_average ms"

