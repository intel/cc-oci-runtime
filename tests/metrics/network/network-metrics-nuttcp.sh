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
# Test inter (docker<->docker) udp network bandwidth
# using nuttcp

SCRIPT_PATH=$(dirname "$(readlink -f "$0")")

source "${SCRIPT_PATH}/../../lib/test-common.bash"

set -e
# Currently default nuttcp has a bug
# see https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=745051
# Image name
image=gabyct/nuttcp
# Measurement time (seconds)
time=5

function setup {
	runtime_docker
	kill_processes_before_start
}

# This script will perform all the measurements using a local setup

# Test single docker->docker udp bandwidth

function udp_bandwidth {
	setup
	bandwidth_result=$(mktemp)
	server_command="/root/nuttcp -u -S"
	$DOCKER_EXE run -tid --name=nuttcp-server ${image} bash > /dev/null
	server_address=$($DOCKER_EXE inspect --format "{{.NetworkSettings.IPAddress}}" $($DOCKER_EXE ps -ql))
	
	client_command="/root/nuttcp -T${time} -u -Ru -i1 -l${bl} ${server_address}"	
	$DOCKER_EXE exec nuttcp-server bash -c "${server_command}"
	$DOCKER_EXE run -ti --rm --name=nuttcp-client ${image} bash -c "${client_command}" > "$bandwidth_result"

	total_bandwidth=$(cat "$bandwidth_result" | tail -1 | cut -d'=' -f2 | awk '{print $(NF-11), $(NF-10)}')
	total_loss=$(cat "$bandwidth_result" | tail -1 | awk '{print $(NF-1), $(NF)}')
	$DOCKER_EXE rm -f nuttcp-server > /dev/null
	rm -f "$bandwidth_result"
}

function udp_default_buffer_size {
	# Test UDP Jumbo (9000 byte MTU) packets
	# Packet header (ICMP+IP) is 28bytes, so maximum payload is 8972 bytes
	# See the nuttcp documentation for more hints:
	# https://fasterdata.es.net/performance-testing/network-troubleshooting-tools/nuttcp/
	bl=8972
	udp_bandwidth
}

function udp_specific_buffer_size {
	# Test UDP standard (1500 byte MTU) packets
	# Even though the packet header (ICMP+IP) is 28 bytes, which would
	# result in 1472 byte frames, the nuttcp documentation recommends
	# use of 1200 byte payloads, so let's run with that for now.
	# See the nuttcp examples.txt for more information:
	# http://nuttcp.net/nuttcp/5.1.3/examples.txt
	bl=1200
	udp_bandwidth
}

udp_default_buffer_size
echo "UDP bandwidth is (${bl} buffer size) : $total_bandwidth"
echo "UDP % of packet loss is (${bl} buffer size) : $total_loss"

udp_specific_buffer_size
echo "UDP bandwidth is (${bl} buffer size) : $total_bandwidth"
echo "UDP % of packet loss is (${bl} buffer size) : $total_loss"
