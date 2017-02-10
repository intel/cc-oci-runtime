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

#This function will verify that swarm is not running or it finishes
function clean_swarm_status() {
        for j in `seq 0 $number_of_attempts`; do
                if $DOCKER_EXE node ls; then
                    $DOCKER_EXE swarm leave --force
                    # "docker swarm leave" is not immediate so it requires some time to finish
                    sleep 5
                else
                        break
                fi
        done
}

setup() {
        source $SRC/test-common.bash
	runtime_docker
	clean_swarm_status
	interfaces=($(readlink /sys/class/net/* | grep -i pci | xargs basename -a))
	swarm_interface_arg=""
	for i in ${interfaces[@]}; do
		if [ "`cat /sys/class/net/${i}/operstate`" == "up" ]; then
                    swarm_interface_arg="--advertise-addr ${i}"
                    break;
                fi
        done
	$DOCKER_EXE swarm init ${swarm_interface_arg}
	$DOCKER_EXE service create --name testswarm1 --replicas $number_of_replicas --publish 8080:80 nginx /bin/bash -c "hostname > /usr/share/nginx/html/hostname; nginx -g \"daemon off;\""
	$DOCKER_EXE service create --name testswarm2 --replicas $number_of_replicas --publish 8082:80 nginx /bin/bash -c "hostname > /usr/share/nginx/html/hostname; nginx -g \"daemon off;\""	
	$DOCKER_EXE service create --name testdns --replicas $number_of_replicas --publish 8084:80 mcastelino/nettools sleep 60000
	while :
        do
                total_tests=`$DOCKER_EXE ps --filter status=running --filter ancestor=nginx:latest --filter ancestor=mcastelino/nettools:latest | wc -l`
                if [ $total_tests -lt $total_number_of_tests ]; then
                    sleep 1
                else
                        break;
                fi
        done
}

@test "DNS test to check Server and IP Address" {
	skip "this is not working (this is related with https://github.com/01org/cc-oci-runtime/issues/578)"
	container_id_dns=`$DOCKER_EXE ps -qaf "name=testdns"`
	expected_virtual_ip_1=`$DOCKER_EXE service inspect testswarm1 -f '{{if eq 1 (len .Endpoint.VirtualIPs)}}{{with index .Endpoint.VirtualIPs 0}}{{.Addr}}{{end}}{{end}}' | cut -d "/" -f1`
	ip_1=$(mktemp)
	$DOCKER_EXE exec -it $container_id_dns nslookup testswarm1 | grep Address | tail -1 | cut -d ':' -f2 | tr -d ' ' > "$ip_1"
	final_ip_1=$(mktemp)
	sed 's/\r//g' $ip_1 > "$final_ip_1"
	cat $final_ip_1 | grep ${expected_virtual_ip_1}
	expected_virtual_ip_2=`$DOCKER_EXE service inspect testswarm2 -f '{{if eq 1 (len .Endpoint.VirtualIPs)}}{{with index .Endpoint.VirtualIPs 0}}{{.Addr}}{{end}}{{end}}' | cut -d "/" -f1`
	ip_2=$(mktemp)
	$DOCKER_EXE exec -it $container_id_dns nslookup testswarm2 | grep Address | tail -1 | cut -d ':' -f2 | tr -d ' ' > "$ip_2"
	final_ip_2=$(mktemp)
	sed 's/\r//g' $ip_2 > "$final_ip_2"
	cat $final_ip_2 | grep ${expected_virtual_ip_2}	
	rm -f $final_ip_1 $final_ip_2 $ip_1 $ip_2
}

@test "DNS test to check hostname" {
	skip "this is not working (this is related with https://github.com/01org/cc-oci-runtime/issues/578)"
	container_id_dns=`$DOCKER_EXE ps -qaf "name=testdns"`
	container_id_1=`$DOCKER_EXE ps -qaf "name=testswarm1"`
	hostname_1=$(mktemp)
	$DOCKER_EXE exec -it $container_id_dns curl testswarm1:/hostname > "$hostname_1"
	final_hostname_1=$(mktemp)
	sed 's/\r//g' $hostname_1 > "$final_hostname_1"
	cat $final_hostname_1 | grep ${container_id_1}
	container_id_2=`$DOCKER_EXE ps -qaf "name=testswarm2"`
	hostname_2=$(mktemp)
	$DOCKER_EXE exec -it $container_id_dns curl testswarm2:/hostname > "$hostname_2"
	final_hostname_2=$(mktemp)
	sed 's/\r//g' $hostname_2 > "$final_hostname_2"
	cat $final_hostname_2 | grep ${container_id_2}
	rm -f $final_hostname_1 $final_hostname_2 $hostname_1 $hostname_2
}

teardown () {
	$DOCKER_EXE service remove testswarm1 testswarm2 testdns
	clean_swarm_status
}
