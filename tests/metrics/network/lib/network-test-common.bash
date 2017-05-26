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

# This file contains functions that are shared among the networking
# tests that are using nuttcp and iperf

SCRIPT_PATH=$(dirname "$(readlink -f "$0")")
source "${SCRIPT_PATH}/../../lib/test-common.bash"

# This function will setup cor as a runtime, clean up the
# environment and create a temporary file where networking
# results will be stored
function setup {
	runtime_docker
	kill_processes_before_start
	result=$(mktemp)
}

# This function can be used in iperf and nuttcp networking tests
function start_server {
	local server_name="$1"
	local image="$2"
	local server_command="$3"
	$DOCKER_EXE run -d --name=${server_name} ${image} sh -c "${server_command}" > /dev/null
	local server_address=$($DOCKER_EXE inspect --format "{{.NetworkSettings.IPAddress}}" ${server_name})
	echo "$server_address"
}

# This function is mainly required in iperf and nuttcp networking tests
# and for PSS measurements it requires to run in detached mode
function start_client {
	local extra_args="$1"
	local client_name="$2"
	local image="$3"
	local client_command="$4"
	$DOCKER_EXE run ${extra_args} --name=${client_name} ${image} sh -c "${client_command}"
}

# This function will remove the server and the temporary file where
# networking results are stored
function clean_environment {
	local server_name="$1"
	$DOCKER_EXE rm -f ${server_name} > /dev/null
	rm -f "$result"
}
