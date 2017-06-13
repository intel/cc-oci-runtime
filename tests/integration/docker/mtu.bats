#!/usr/bin/env bats
# *-*- Mode: sh; sh-basic-offset: 8; indent-tabs-mode: nil -*-*

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

# MTU testing using Swarm, this will check the different interfaces

SRC="${BATS_TEST_DIRNAME}/../../lib/"
# number of replicas to launch
number_of_replicas=1
# number of attempts
number_of_attempts=5

#Name of service to test swarm
SERVICE_NAME="testswarm"
# timeout to wait for swarm commands (seconds)
timeout=120

setup() {
	source $SRC/test-common.bash
	runtime_docker
	kill_processes_before_start
	clean_swarm_status
	interfaces=($(readlink /sys/class/net/* | grep -i pci | xargs basename -a))
	swarm_interface_arg=""
	for i in ${interfaces[@]}; do
		if [ "`cat /sys/class/net/${i}/operstate`" == "up" ]; then
			swarm_interface_arg="--advertise-addr ${i}"
			break;
		fi
	done

	cmd='$DOCKER_EXE swarm init ${swarm_interface_arg}'
	info "running '$cmd'"
	eval $cmd

	info 'running $DOCKER_EXE service create --name "$SERVICE_NAME" --replicas "'
	$DOCKER_EXE service create --name "$SERVICE_NAME" \
	--replicas "$number_of_replicas" \
	--publish 8080:80 nginx /bin/bash \
	-c "hostname > /usr/share/nginx/html/hostname; nginx -g \"daemon off;\""
	info 'check_replicas ...'
	check_swarm_replicas "$number_of_replicas" "$SERVICE_NAME" "$timeout"
}

@test "Checking MTU values in different interfaces" {
	ip_addresses=$(mktemp)
	container_id=`$DOCKER_EXE ps -qf "name=${SERVICE_NAME}"`
	network_settings_file=`$DOCKER_EXE inspect "$container_id" | grep "SandboxKey" | cut -d ':' -f2 | cut -d '"' -f2`
	[ -f "$network_settings_file" ]
	nsenter --net="$network_settings_file" ip a > "$ip_addresses"
	mtu_value_ceth0=`grep -w "ceth0" "$ip_addresses" | grep "mtu" | cut -d ' ' -f5`
	mtu_value_eth0=`grep -w "eth0" "$ip_addresses" | grep "mtu" | cut -d ' ' -f5`
	[ "$mtu_value_eth0" = "$mtu_value_ceth0" ]
	mtu_value_eth1=`grep -w "eth1" "$ip_addresses" | grep "mtu" | cut -d ' ' -f5`
	mtu_value_ceth1=`grep -w "ceth1" "$ip_addresses" | grep "mtu" | cut -d ' ' -f5`
	[ "$mtu_value_eth1" = "$mtu_value_ceth1" ]
	rm -f "$ip_addresses"
}

teardown() {
	$DOCKER_EXE service remove "${SERVICE_NAME}"
	clean_swarm_status
	check_no_processes_up
}
