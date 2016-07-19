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

@test "pause without container id" {
	run $COR pause
	echo $output
        [ "$status" -ne 0 ]
	[[ "${output}" == "Usage: pause <container-id>" ]]
}

@test "pause with invalid container id" {
	run $COR pause FOO
        [ "$status" -ne 0 ]
	[[ "${output}" =~ "failed to parse json file:" ]]
}

@test "resume without container id" {
	run $COR resume
	echo $output
        [ "$status" -ne 0 ]
	[[ "${output}" == "Usage: resume <container-id>" ]]
}

@test "resume with invalid container id" {
	run $COR resume FOO
        [ "$status" -ne 0 ]
	[[ "${output}" =~ "failed to parse json file:" ]]
}

@test "start then pause and resume" {
	workload_cmd "sh"

	cmd="$COR run --console --bundle $BUNDLE_DIR $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "running"

	cmd="$COR pause $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "paused"

	cmd="$COR resume $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "running"

	#Kill container after test
	cmd="$COR attach $container_id"
	echo "exit" |  run_cmd "$cmd" "0" "$COR_TIMEOUT"
}
