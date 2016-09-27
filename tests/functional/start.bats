#!/usr/bin/env bats
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
#

load common

function setup() {
	setup_common
	#Start use Clear Containers
	check_ccontainers
	#Default timeout for cor commands
	COR_TIMEOUT=5
	container_id="tests_id"
}

function teardown() {
	cleanup_common
}

@test "start without container id" {
	run $COR start
	[ "$status" -ne 0 ]
	[[ "${output}" == "Usage: start <container-id>" ]]
}

@test "start with invalid container id" {
	run $COR start FOO
	[ "$status" -ne 0 ]
	[[ "${output}" =~ "failed to parse json file:" ]]
}

@test "start detach then attach" {
	#Start a container with "sh"
	workload_cmd "sh"

	cmd="$COR create  --console --bundle $BUNDLE_DIR $container_id"
	#Wait $COR_TIMEOUT before kill and fail the tests
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "created"

	cmd="$COR start $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "running"

	cmd="$COR attach $container_id"
	#Wait $COR_TIMEOUT before kill and fail the tests
	echo "exit" |  run_cmd "$cmd" "0" "$COR_TIMEOUT"
}

@test "start then stop" {
	workload_cmd "sh"

	cmd="$COR create  --console --bundle $BUNDLE_DIR $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "created"

	cmd="$COR start $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "running"

	cmd="$COR stop $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"

	# restore original IFS to work around bats bug causing test
	# failure.
	[ -n "$OLD_IFS" -a "$IFS" != "$OLD_IFS" ] && IFS=$OLD_IFS

	run $COR list
	[ "$status" -eq 0 ]
	[[ ${output} =~ ID\ +PID\ +STATUS\ +BUNDLE\ +CREATED+ ]]
}

@test "run without params" {
	run $COR run
	[ "$status" -ne 0 ]
	[[ "${output}" == "Usage: run <container-id>" ]]
}

@test "run detach then attach" {
	workload_cmd "sh"

	cmd="$COR run --console --bundle $BUNDLE_DIR $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "running"

	cmd="$COR attach $container_id"
	echo "exit" |  run_cmd "$cmd" "0" "$COR_TIMEOUT"
}

@test "run detach pid file" {
	workload_cmd "true"

	cmd="$COR run --pid-file ${COR_ROOT_DIR}/pid --console --bundle $BUNDLE_DIR $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	[ -f "${COR_ROOT_DIR}/pid" ]
}
