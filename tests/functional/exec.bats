#!/usr/bin/env bats
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


@test "start without params" {
	run $COR exec
	[ "$status" -ne 0 ]
	[[ "${output}" == "Usage: exec <container-id> <cmd> [args]" ]]
}

@test "start with invalid container id" {
	run $COR exec FOO
	[ "$status" -ne 0 ]
}

@test "exec false" {
	workload_cmd "sh"

	cmd="$COR run -d --bundle $BUNDLE_DIR $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "running"

	cmd="$COR exec $container_id false"
	run_cmd "$cmd" "1" "$COR_TIMEOUT"
	testcontainer "$container_id" "running"

	cmd="$COR kill --all $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "killed"

	cmd="$COR delete $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	verify_runtime_dirs "$container_id" "deleted"
}

@test "exec false using json file" {
	workload_cmd "sh"

	cmd="$COR run -d --bundle $BUNDLE_DIR $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "running"

	cmd="$COR exec -p ${TEST_DATA_DIR}/exec_false.json $container_id"
	run_cmd "$cmd" "1" "$COR_TIMEOUT"
	testcontainer "$container_id" "running"

	cmd="$COR kill --all $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "killed"

	cmd="$COR delete $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	verify_runtime_dirs "$container_id" "deleted"
}


@test "exec detach pid file" {
	workload_cmd "sh"

	cmd="$COR run -d --pid-file ${COR_ROOT_DIR}/pid --bundle $BUNDLE_DIR $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	[ -f "${COR_ROOT_DIR}/pid" ]
	testcontainer "$container_id" "running"

	cmd="$COR exec -d --pid-file ${COR_ROOT_DIR}/pidexec $container_id sh"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	[ -f "${COR_ROOT_DIR}/pidexec" ]
	testcontainer "$container_id" "running"

	cmd="$COR kill --all $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "killed"

	cmd="$COR delete $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	verify_runtime_dirs "$container_id" "deleted"
}

@test "exec detach using json file" {
	workload_cmd "sh"

	cmd="$COR run -d --bundle $BUNDLE_DIR $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "running"

	cmd="$COR exec -d -p ${TEST_DATA_DIR}/exec.json $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "running"

	cmd="$COR kill --all $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "killed"

	cmd="$COR delete $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	verify_runtime_dirs "$container_id" "deleted"
}
