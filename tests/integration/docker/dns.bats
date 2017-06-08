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

# DNS tests using Swarm

SRC="${BATS_TEST_DIRNAME}/../../lib/"
# number of replicas to launch
number_of_replicas=1
# total number of tests to launch
total_number_of_tests=3
# number of attempts
number_of_attempts=5
# timeout to wait for swarm commands (seconds)
timeout=120
SERVICE1_NAME="nginx_service1"
SERVICE2_NAME="nginx_service2"
DNS_TEST_SERVICE_NAME="dnstest"

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
	info "init swarm"
	$DOCKER_EXE swarm init ${swarm_interface_arg}

	info "create service ${SERVICE1_NAME}"
	$DOCKER_EXE service create \
	--name "${SERVICE1_NAME}" \
	--replicas $number_of_replicas \
	--publish 8080:80 nginx /bin/bash \
	-c "hostname > /usr/share/nginx/html/hostname; nginx -g \"daemon off;\""

	info "create service ${SERVICE2_NAME}"
	$DOCKER_EXE service create \
	--name "${SERVICE2_NAME}" \
	--replicas $number_of_replicas \
	--publish 8082:80 nginx /bin/bash \
	-c "hostname > /usr/share/nginx/html/hostname; nginx -g \"daemon off;\""

	info "create service ${DNS_TEST_SERVICE_NAME}"
	$DOCKER_EXE service create \
	--name "${DNS_TEST_SERVICE_NAME}" \
	--replicas $number_of_replicas \
	--publish 8084:80 \
	mcastelino/nettools sleep 60000

	info "check service ${SERVICE1_NAME}"
	check_swarm_replicas "$number_of_replicas" "${SERVICE1_NAME}" "$timeout"
	info "check service ${SERVICE2_NAME}"
	check_swarm_replicas "$number_of_replicas" "${SERVICE2_NAME}" "$timeout"
	info "check service ${DNS_TEST_SERVICE_NAME}"
	check_swarm_replicas "$number_of_replicas" "${DNS_TEST_SERVICE_NAME}" "$timeout"

}

@test "DNS test to check Server and IP Address" {
	skip "this is not working (this is related with https://github.com/01org/cc-oci-runtime/issues/578)"
	container_id_dns=`$DOCKER_EXE ps -qf "name=${DNS_TEST_SERVICE_NAME}"`
	expected_virtual_ip_1=`$DOCKER_EXE service inspect "${SERVICE1_NAME}" -f '{{if eq 1 (len .Endpoint.VirtualIPs)}}{{with index .Endpoint.VirtualIPs 0}}{{.Addr}}{{end}}{{end}}' | cut -d "/" -f1`
	ip_1=$(mktemp)
	$DOCKER_EXE exec -it $container_id_dns nslookup "${SERVICE1_NAME}" | grep Address | tail -1 | cut -d ':' -f2 | tr -d ' ' > "$ip_1"
	final_ip_1=$(mktemp)
	sed 's/\r//g' $ip_1 > "$final_ip_1"
	cat $final_ip_1 | grep ${expected_virtual_ip_1}
	expected_virtual_ip_2=`$DOCKER_EXE service inspect "${SERVICE2_NAME}" -f '{{if eq 1 (len .Endpoint.VirtualIPs)}}{{with index .Endpoint.VirtualIPs 0}}{{.Addr}}{{end}}{{end}}' | cut -d "/" -f1`
	ip_2=$(mktemp)
	$DOCKER_EXE exec -it $container_id_dns nslookup "${SERVICE2_NAME}" | grep Address | tail -1 | cut -d ':' -f2 | tr -d ' ' > "$ip_2"
	final_ip_2=$(mktemp)
	sed 's/\r//g' $ip_2 > "$final_ip_2"
	cat $final_ip_2 | grep ${expected_virtual_ip_2}	
	rm -f $final_ip_1 $final_ip_2 $ip_1 $ip_2
}

@test "DNS test to check hostname" {
	skip "this is not working (this is related with https://github.com/01org/cc-oci-runtime/issues/578)"
	container_id_dns=`$DOCKER_EXE ps -qf "name=${DNS_TEST_SERVICE_NAME}"`
	container_id_1=`$DOCKER_EXE ps -qf "name=${SERVICE1_NAME}"`
	hostname_1=$(mktemp)
	$DOCKER_EXE exec -it $container_id_dns curl "${SERVICE1_NAME}:/hostname" > "$hostname_1"
	final_hostname_1=$(mktemp)
	sed 's/\r//g' $hostname_1 > "$final_hostname_1"
	cat $final_hostname_1 | grep ${container_id_1}
	container_id_2=`$DOCKER_EXE ps -qaf "name=${SERVICE2_NAME}"`
	hostname_2=$(mktemp)
	$DOCKER_EXE exec -it $container_id_dns curl "${SERVICE2_NAME}:/hostname" > "$hostname_2"
	final_hostname_2=$(mktemp)
	sed 's/\r//g' $hostname_2 > "$final_hostname_2"
	cat $final_hostname_2 | grep ${container_id_2}
	rm -f $final_hostname_1 $final_hostname_2 $hostname_1 $hostname_2
}

teardown () {
	$DOCKER_EXE service remove "${SERVICE1_NAME}" "${SERVICE2_NAME}" "${DNS_TEST_SERVICE_NAME}"
	clean_swarm_status
	check_no_processes_up
}
