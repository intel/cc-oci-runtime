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
source "${SCRIPT_PATH}/lib/network-test-common.bash"

set -e

# This script will perform all the measurements using a local setup

# Test single docker->docker udp bandwidth

function udp_bandwidth {
	# Currently default nuttcp has a bug
	# see https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=745051
	# Image name
	local image=gabyct/nuttcp
	# Measurement time (seconds)
	local time=5
	# Name of the containers
	local server_name="network-server"
	local client_name="network-client"
	# Arguments to run the client
	local extra_args="-ti --rm"

	setup
	local server_command="sleep 30"
	local server_address=$(start_server "$server_name" "$image" "$server_command")

	local client_command="/root/nuttcp -T${time} -u -Ru -i1 -l${bl} ${server_address}"
	local server_command="/root/nuttcp -u -S"
	$DOCKER_EXE exec ${server_name} sh -c "${server_command}"
	start_client "$extra_args" "$client_name" "$image" "$client_command" > "$result"

	local total_bandwidth=$(cat "$result" | tail -1 | cut -d'=' -f2 | awk '{print $(NF-11), $(NF-10)}')	
	local total_loss=$(cat "$result" | tail -1 | awk '{print $(NF-1), $(NF)}')
	echo "UDP bandwidth is (${bl} buffer size) : $total_bandwidth"
	echo "UDP % of packet loss is (${bl} buffer size) : $total_loss"
	clean_environment "$server_name"
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

udp_specific_buffer_size
