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
# Test inter-container network bandwidth and jitter using iperf3

SCRIPT_PATH=$(dirname "$(readlink -f "$0")")

source "${SCRIPT_PATH}/../../lib/test-common.bash"

# Port number where the server will run
port=5201:5201
# Image name
image=gabyct/network
# Measurement time (seconds)
time=5

function setup {
	runtime_docker
	kill_processes_before_start
}

# This script will perform all the measurements using a local setup using iperf3

# Test single direction TCP bandwith
function iperf3_bandwidth {
	setup
	bandwidth_result=$(mktemp)
	$DOCKER_EXE run -d --name=iperf3-server ${image} bash -c "mount -t ramfs -o size=20M ramfs /tmp && iperf3 -p ${port} -s" > /dev/null && \
	server_address=`$DOCKER_EXE inspect --format "{{.NetworkSettings.IPAddress}}" $($DOCKER_EXE ps -ql)` && \
	$DOCKER_EXE run -ti --rm --name=iperf3-client ${image} bash -c "mount -t ramfs -o size=20M ramfs /tmp && iperf3 -c ${server_address} -t ${time}" > "$bandwidth_result"
	total_bandwidth=`cat $bandwidth_result | tail -n 3 | head -1 | awk '{print $(NF-2), $(NF-1)}'`
	$DOCKER_EXE rm -f iperf3-server > /dev/null
	rm -f $bandwidth_result
}

# Test jitter on single direction UDP
function iperf3_jitter {
	setup
	jitter_result=$(mktemp)
	$DOCKER_EXE run -d --name=iperf3-server ${image} bash -c "mount -t ramfs -o size=20M ramfs /tmp && iperf3 -s -V" > /dev/null && \
	server_address=`$DOCKER_EXE inspect --format "{{.NetworkSettings.IPAddress}}" $($DOCKER_EXE ps -ql)` && \
	$DOCKER_EXE run -ti --rm --name=iperf3-client ${image} bash -c "mount -t ramfs -o size=20M ramfs /tmp && iperf3 -c ${server_address} -u -t ${time}"  > "$jitter_result"
	total_jitter=`cat $jitter_result | tail -n 4 | head -1 | awk '{print $(NF-4), $(NF-3)}'`
	$DOCKER_EXE rm -f iperf3-server > /dev/null
	rm -f $jitter_result
}

# Run bi-directional TCP test, and extract results for both directions
function iperf3_bidirectional_bandwidth_client_server {
	setup
	bidirectional_bandwidth_result=$(mktemp)
	$DOCKER_EXE run -d --name=iperf3-server ${image} bash -c "mount -t ramfs -o size=20M ramfs /tmp && iperf3 -p ${port} -s" > /dev/null && \
	server_address=`$DOCKER_EXE inspect --format "{{.NetworkSettings.IPAddress}}" $($DOCKER_EXE ps -ql)` && \
	$DOCKER_EXE run -ti --rm --name=iperf3-client ${image} bash -c "mount -t ramfs -o size=20M ramfs /tmp && iperf3 -c ${server_address} -d -t ${time}" > "$bidirectional_bandwidth_result"
	total_bidirectional_client_bandwidth=`cat $bidirectional_bandwidth_result | tail -n 3 | head -1 | awk '{print $(NF-2), $(NF-1)}'`
	total_bidirectional_server_bandwidth=`cat $bidirectional_bandwidth_result | tail -n 4 | head -1 | awk '{print $(NF-3), $(NF-2)}'`
	$DOCKER_EXE rm -f iperf3-server > /dev/null
	rm -f $bidirectional_bandwidth_result
}

echo "Currently this script is using ramfs for tmp (see https://github.com/01org/cc-oci-runtime/issues/152)"

iperf3_bandwidth
echo "Network bandwidth is : $total_bandwidth"

iperf3_jitter
echo "Network jitter is : $total_jitter"

iperf3_bidirectional_bandwidth_client_server
echo "Network bidirectional bandwidth (client to server) is : $total_bidirectional_client_bandwidth"
echo "Network bidirectional bandwidth (server to client) is : $total_bidirectional_server_bandwidth"

