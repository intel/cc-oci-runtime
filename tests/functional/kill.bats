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

@test "kill without container id" {
	run $COR kill
        [ "$status" -ne 0 ]
	[[ "${output}" == "Usage: kill <container-id>" ]]
}

@test "kill with invalid container id" {
	run $COR kill FOO
        [ "$status" -ne 0 ]
	[[ "${output}" =~ "failed to parse json file:" ]]
}

@test "start then kill (implicit signal)" {
	workload_cmd "sh"

	cmd="$COR create  --console --bundle $BUNDLE_DIR $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "created"

	cmd="$COR start $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "running"

	cmd="$COR kill $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "stopped"
}

@test "start then kill (short symbolic signal)" {
	workload_cmd "sh"

	cmd="$COR create  --console --bundle $BUNDLE_DIR $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "created"

	cmd="$COR start $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "running"

	# specify invalid signal name
	cmd="$COR kill $container_id FOOBAR"
	run_cmd "$cmd" "1" "$COR_TIMEOUT"

	cmd="$COR kill $container_id TERM"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "stopped"
}

@test "start then kill (full symbolic signal)" {
	workload_cmd "sh"

	cmd="$COR create  --console --bundle $BUNDLE_DIR $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "created"

	cmd="$COR start $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "running"

	# specify invalid signal name
	cmd="$COR kill $container_id SIGFOOBAR"
	run_cmd "$cmd" "1" "$COR_TIMEOUT"

	cmd="$COR kill $container_id SIGTERM"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "stopped"
}

@test "start then kill (numeric signal)" {
	workload_cmd "sh"

	cmd="$COR create  --console --bundle $BUNDLE_DIR $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "created"

	cmd="$COR start $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "running"

	# specify invalid signal number
	cmd="$COR kill $container_id 123456"
	run_cmd "$cmd" "1" "$COR_TIMEOUT"

	cmd="$COR kill $container_id 15"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "stopped"
}
