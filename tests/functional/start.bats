#!/usr/bin/env bats

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

	run $COR list
	[ "$status" -eq 0 ]
	[[ ${output} =~ ID\ +PID\ +STATUS\ +BUNDLE\ +CREATED+ ]]
}
