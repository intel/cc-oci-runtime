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
#  Test inter (docker<->docker) network bandwidths and jitters
# using iperf2

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

# Test single TCP docker->docker iperf bandwidth
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

# Test single UDP docker->docker jitter
function jitter {
        setup
        jitter_result=$(mktemp)
        $DOCKER_EXE run -d --name=iperf-server ${image} bash -c "iperf -p ${port} -u -s" > /dev/null && \
        server_address=`$DOCKER_EXE inspect --format "{{.NetworkSettings.IPAddress}}" $($DOCKER_EXE ps -ql)` && \
        $DOCKER_EXE run -ti --rm --name=iperf-client ${image} bash -c "iperf -c ${server_address} -u -t ${time}" > "$jitter_result"
        total_jitter=`cat $jitter_result | tail -1 | awk '{print $(NF-4), $(NF-3)}'`
        $DOCKER_EXE rm -f iperf-server > /dev/null
        rm -f $jitter_result
}

# Test TCP docker->docker bandwidth with multiple network connections (parallelism)
function bandwidth_multiple_tcp_connections {
        setup
        number_tcp_connections=8
        multiple_tcp_result=$(mktemp)
        $DOCKER_EXE run -d --name=iperf-server ${image} bash -c "iperf -p ${port} -s" > /dev/null && \
        server_address=`$DOCKER_EXE inspect --format "{{.NetworkSettings.IPAddress}}" $($DOCKER_EXE ps -ql)` && \
        $DOCKER_EXE run -ti --rm --name=iperf-client ${image} bash -c "iperf -c ${server_address} -P ${number_tcp_connections} -t ${time}"  > "$multiple_tcp_result"
        total_multiple_tcp=`cat $multiple_tcp_result | tail -1 | awk '{print $(NF-1), $NF}'`
        $DOCKER_EXE rm -f iperf-server > /dev/null
        rm -f $multiple_tcp_result
}

# Test TCP bandwidth bi-directionally (docker_iperf_server<->docker_iperf_client)
# Extract bandwidth results for both directions from the one test
function bidirectional_bandwidth_server_client {
        setup
        bidirectional_bandwidth_result=$(mktemp)
        $DOCKER_EXE run -d --name=iperf-server ${image} bash -c "iperf -p ${port} -s" > /dev/null && \
        server_address=`$DOCKER_EXE inspect --format "{{.NetworkSettings.IPAddress}}" $($DOCKER_EXE ps -ql)` && \
        $DOCKER_EXE run -ti --rm --name=iperf-client ${image} bash -c "iperf -c ${server_address} -d -t ${time}" > "$bidirectional_bandwidth_result"
        total_bidirectional_server_bandwidth=`cat $bidirectional_bandwidth_result | tail -1 | awk '{print $(NF-1), $NF}'`
        total_bidirectional_client_bandwidth=`cat $bidirectional_bandwidth_result | tail -n 2 | head -1 | awk '{print $(NF-1), $NF}'`
        $DOCKER_EXE rm -f iperf-server > /dev/null
        rm -f $bidirectional_bandwidth_result
}

bandwidth
echo "Network bandwidth is : $total_bandwidth"

jitter
echo "Network jitter is : $total_jitter"

bandwidth_multiple_tcp_connections
echo "Network bandwidth for multiple TCP connections is : $total_multiple_tcp"

bidirectional_bandwidth_server_client
echo "Bi-directional network bandwidth is (client to server) : $total_bidirectional_client_bandwidth"
echo "Bi-directional network bandwidth is (server to server) : $total_bidirectional_server_bandwidth"
