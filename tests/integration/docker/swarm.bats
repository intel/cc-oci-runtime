#!/usr/bin/env bats
# *-*- Mode: sh; sh-basic-offset: 8; indent-tabs-mode: nil -*-*

#  This file is part of cc-oci-runtime.
#
#  Copyright (C) 2016 Intel Corporation
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

# Swarm testing

SRC="${BATS_TEST_DIRNAME}/../../integration/lib/"
# maximum number of replicas that will be launch
number_of_replicas=15
url=http://127.0.0.1:8080/hostname
# number of attemps to obtain the hostname
# of the replicas using curl
number_of_attemps=5
declare -a REPLICAS

setup() {
	source $SRC/common.bash
	cleanDockerPs
	runtimeDocker
	$DOCKER_EXE swarm init
}

@test "check that the replicas' names are different" {
	# currently using mcastelino/nginx but it will be
	# modified when nginx image is working
	$DOCKER_EXE service create --name testswarm --replicas $number_of_replicas --publish 8080:80 mcastelino/nginx /bin/bash -c "hostname > /usr/share/nginx/html/hostname; nginx -g \"daemon off;\"" 2> /dev/null
	while [ `docker ps -a | tail -n +2 | egrep "Up [0-9]*"|  wc -l`  -lt  $number_of_replicas ]; do
		sleep 1
	done
	# this will help to obtain the hostname of 
	# the replicas from the curl
        unset http_proxy
        for j in `seq 0 $number_of_attemps`; do
	        for i in `seq 0 $((number_of_replicas-1))`; do
		        # this will help that bat does not exit incorrectly 
			# when curl fails in one of the attemps
			set +e
		        REPLICAS[$i]="$(curl $url 2> /dev/null)"
	                set -e
                done
		non_empty_elements="$(echo ${REPLICAS[@]} | egrep -o "[[:space:]]+" | wc -l)"
                if [ "$non_empty_elements" == "$((number_of_replicas-1))" ]; then
                	break
                fi
		# this will give enough time between attemps
                sleep 5
        done
	for i in `seq 0 $((number_of_replicas-1))`; do
                if [ -z "${REPLICAS[$i]}" ]; then
                    false
                fi
		for j in `seq $((i+1)) $((number_of_replicas-1))`; do
			if [ "${REPLICAS[$i]}" == "${REPLICAS[$j]}" ]; then
				false
			fi
		done
	done 
}

teardown () {
	$DOCKER_EXE service remove testswarm
	$DOCKER_EXE swarm leave --force
}
