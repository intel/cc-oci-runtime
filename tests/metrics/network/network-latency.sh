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

SCRIPT_PATH=$(dirname "$(readlink -f "$0")")

source "${SCRIPT_PATH}/../../lib/test-common.bash"
source "${SCRIPT_PATH}/lib/network-test-common.bash"

set -e

# This script will perform all the measurements using a local setup

# Test latency docker<->docker using ping

function latency {
	# Image name (ping installed by default)
	local image=busybox
	# Number of packets (sent)
	local number=10
	# Name of the containers
	local server_name="network-server"
	local client_name="network-client"
	# Arguments to run the client
	local extra_args="-ti --rm"

	setup
	local server_command="sleep 30"
	local server_address=$(start_server "$server_name" "$image" "$server_command")

	local client_command="ping -c ${number} ${server_address}"
	start_client "$extra_args" "$client_name" "$image" "$client_command" > "$result"

	local latency_average=$(cat $result | grep avg | tail -1 | awk '{print $4}' | cut -d '/' -f 2)
	echo "The average latency is : $latency_average ms"
	clean_environment "$server_name"
}

latency
