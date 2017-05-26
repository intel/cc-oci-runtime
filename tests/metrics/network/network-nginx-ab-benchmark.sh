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
#  Test (host<->docker) network performance using
#  an nginx container and running the Apache 
#  benchmarking tool in the host to calculate the 
#  requests per second

SCRIPT_PATH=$(dirname "$(readlink -f "$0")")

source "${SCRIPT_PATH}/../../lib/test-common.bash"

# Ports where it will run
port=80:80
# Image name
image=nginx
# Url
url=localhost:80
# Number of requests to perform
# (large number to reduce standard deviation)
requests=100000
# Number of multiple requests to make
# (large number to reduce standard deviation)
concurrency=100

function setup {
	runtime_docker
	kill_processes_before_start
}

function check_environment {
	ab -V
	if [ $? -ne 0 ]
	then
		echo "Please install the Apache HTTP server benchmarking tool (ab)"
		exit 1
	fi
}

# This script will perform all the measurements using a local setup

# Test single host<->docker requests per second using nginx and ab

function nginx_ab_networking {
	setup
	check_environment
	container_name="docker-nginx"
	total_requests=$(mktemp)

	$DOCKER_EXE run -d --name ${container_name} -p ${port} ${image} > /dev/null
	sleep_secs=2
	echo >&2 "WARNING: sleeping for $sleep_secs seconds (see https://github.com/01org/cc-oci-runtime/issues/828)"
	sleep "$sleep_secs"
	ab -n ${requests} -c ${concurrency} http://${url}/ > "$total_requests"

	result=$(cat "$total_requests" | grep "Requests per second" | cut -d ':' -f2 | sed 's/ //g' | cut -d '[' -f1)
	$DOCKER_EXE rm -f ${container_name} > /dev/null	
	rm -f "$total_requests"
}

nginx_ab_networking
echo "The total of requests per second is : $result"
