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
source "${SCRIPT_PATH}/lib/network-test-common.bash"

# Port number where the server will run
port=5201:5201
# Image name
image=gabyct/network
# Measurement time (seconds)
time=5
# Name of the containers
server_name="network-server"
client_name="network-client"
# Arguments to run the client
extra_args="-ti --rm"

set -e

# This script will perform all the measurements using a local setup using iperf3

# Test single direction TCP bandwith
function iperf3_bandwidth {
	setup
	local server_command="mount -t ramfs -o size=20M ramfs /tmp && iperf3 -p ${port} -s"
	local server_address=$(start_server "$server_name" "$image" "$server_command")

	local client_command="mount -t ramfs -o size=20M ramfs /tmp && iperf3 -c ${server_address} -t ${time}"
	start_client "$extra_args" "$client_name" "$image" "$client_command" > "$result"

	local total_bandwidth=$(cat $result | tail -n 3 | head -1 | awk '{print $(NF-2), $(NF-1)}')
	echo "Network bandwidth is : $total_bandwidth"
	clean_environment "$server_name"
}

# Test jitter on single direction UDP
function iperf3_jitter {
	setup
	local server_command="mount -t ramfs -o size=20M ramfs /tmp && iperf3 -s -V"
	local server_address=$(start_server "$server_name" "$image" "$server_command")

	local client_command="mount -t ramfs -o size=20M ramfs /tmp && iperf3 -c ${server_address} -u -t ${time}"
	start_client "$extra_args" "$client_name" "$image" "$client_command" > "$result"

	local total_jitter=$(cat $result | tail -n 4 | head -1 | awk '{print $(NF-4), $(NF-3)}')
	echo "Network jitter is : $total_jitter"
	clean_environment "$server_name"
}

# Run bi-directional TCP test, and extract results for both directions
function iperf3_bidirectional_bandwidth_client_server {
	setup
	local server_command="mount -t ramfs -o size=20M ramfs /tmp && iperf3 -p ${port} -s"
	local server_address=$(start_server "$server_name" "$image" "$server_command")

	local client_command="mount -t ramfs -o size=20M ramfs /tmp && iperf3 -c ${server_address} -d -t ${time}"
	start_client "$extra_args" "$client_name" "$image" "$client_command" > "$result"

	local total_bidirectional_client_bandwidth=$(cat $result | tail -n 3 | head -1 | awk '{print $(NF-2), $(NF-1)}')
	local total_bidirectional_server_bandwidth=$(cat $result | tail -n 4 | head -1 | awk '{print $(NF-3), $(NF-2)}')
	echo "Network bidirectional bandwidth (client to server) is : $total_bidirectional_client_bandwidth"
	echo "Network bidirectional bandwidth (server to client) is : $total_bidirectional_server_bandwidth"
	clean_environment "$server_name"
}

echo "Currently this script is using ramfs for tmp (see https://github.com/01org/cc-oci-runtime/issues/152)"

iperf3_bandwidth

iperf3_jitter

iperf3_bidirectional_bandwidth_client_server
