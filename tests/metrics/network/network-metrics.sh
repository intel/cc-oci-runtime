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

SCRIPT_PATH=$(dirname "$(readlink -f "$0")")

source "${SCRIPT_PATH}/../../lib/test-common.bash"

# Port number where the server will run
port=5001:5001
# Image name
image=gabyct/network
# Measurement time (seconds)
time=5

function setup {
        runtime_docker
        kill_processes_before_start
}

# This script will perform all the measurements using a local setup

function bandwidth {
        setup
        bandwidth_result=$(mktemp)
        $DOCKER_EXE run -d --name=iperf-server ${image} bash -c "iperf -p ${port} -s" > /dev/null && \
	server_address=`$DOCKER_EXE inspect --format "{{.NetworkSettings.IPAddress}}" $($DOCKER_EXE ps -ql)` && \
        $DOCKER_EXE run -ti --rm --name=iperf-client ${image} bash -c "iperf -c ${server_address} -t ${time}" > "$bandwidth_result"
        total_bandwidth=`cat $bandwidth_result | tail -1 | awk '{print $(NF-1), $NF}'`
        $DOCKER_EXE rm -f iperf-server > /dev/null
        rm -f $bandwidth_result
}

function jitter {
        setup
        jitter_result=$(mktemp)
        $DOCKER_EXE run -d --name=iperf-server ${image} bash -c "iperf -p ${port} -u -s" > /dev/null && \
	server_address=`$DOCKER_EXE inspect --format "{{.NetworkSettings.IPAddress}}" $($DOCKER_EXE ps -ql)` && \
        $DOCKER_EXE run -ti --rm --name=iperf-client ${image} bash -c "iperf -c ${server_address} -u -t ${time}" > "$jitter_result"
        total_jitter=`cat $jitter_result | tail -1  | awk '{print $(NF-4), $(NF-3)}'`
        $DOCKER_EXE rm -f iperf-server > /dev/null
        rm -f $jitter_result
}

function bandwidth_multiple_tcp_connections {
        setup
        number_tcp_connections=8
        multiple_tcp_result=$(mktemp)
        $DOCKER_EXE run -d --name=iperf-server ${image} bash -c "iperf -p ${port} -s" > /dev/null && \
	server_address=`$DOCKER_EXE inspect --format "{{.NetworkSettings.IPAddress}}" $($DOCKER_EXE ps -ql)` && \
        $DOCKER_EXE run -ti --rm --name=iperf-client ${image} bash -c "iperf -c ${server_address} -P ${multiple_tcp_result} -t ${time}"  > "$multiple_tcp_result"
        total_multiple_tcp=`cat $multiple_tcp_result | tail -1 | awk '{print $(NF-1), $NF}'`
        $DOCKER_EXE rm -f iperf-server > /dev/null
        rm -f $multiple_tcp_result
}

function bidirectional_bandwidth_remote_local {
        setup
        bidirectional_bandwidth_result=$(mktemp)
        $DOCKER_EXE run -d --name=iperf-server ${image} bash -c "iperf -p ${port} -s" > /dev/null && \
	server_address=`$DOCKER_EXE inspect --format "{{.NetworkSettings.IPAddress}}" $($DOCKER_EXE ps -ql)` && \
        $DOCKER_EXE run -ti --rm --name=iperf-client ${image} bash -c "iperf -c ${server_address} -d -t ${time}" > "$bidirectional_bandwidth_result"
        total_bidirectional_bandwidth=`cat $bidirectional_bandwidth_result | tail -1 | awk '{print $(NF-1), $NF}'`
        $DOCKER_EXE rm -f iperf-server > /dev/null
        rm -f $bidirectional_bandwidth_result
}

function bidirectional_bandwidth_local_remote {
	setup
	bidirectional_bandwidth_result=$(mktemp)
	$DOCKER_EXE run -d --name=iperf-server ${image} bash -c "iperf -p ${port} -s" > /dev/null && \
	server_address=`$DOCKER_EXE inspect --format "{{.NetworkSettings.IPAddress}}" $($DOCKER_EXE ps -ql)` && \
	$DOCKER_EXE run -ti --rm --name=iperf-client ${image} bash -c "iperf -c ${server_address} -d -t ${time}" > "$bidirectional_bandwidth_result"
	total_bidirectional_bandwidth=`cat $bidirectional_bandwidth_result | tail -n 2 | head -1 | awk '{print $(NF-1), $NF}'`
	$DOCKER_EXE rm -f iperf-server > /dev/null
	rm -f $bidirectional_bandwidth_result
}

bandwidth
echo "Network bandwidth is : $total_bandwidth"

jitter
echo "Network jitter is : $total_jitter"

bandwidth_multiple_tcp_connections
echo "Network bandwidth for multiple TCP connections is : $total_multiple_tcp"

bidirectional_bandwidth_remote_local
echo "Bi-directional network bandwidth is (remote to local host) : $total_bidirectional_bandwidth"

bidirectional_bandwidth_local_remote
echo "Bi-directional network bandwidth is (local to remote host) : $total_bidirectional_bandwidth"
