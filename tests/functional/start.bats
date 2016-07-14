#!/usr/bin/env bats

load common

function setup() {
	qemu_have_pclite
	setup_bundle
	container_id=$(tr -cd '[:xdigit:]' < /dev/urandom|head -c 10 |tr '[A-Z]' '[a-z]')
	COR_ROOT_DIR=$(mktempd)
	COR_GLOBAL_OPTIONS="--debug --root $COR_ROOT_DIR"
	COR="$COR $COR_GLOBAL_OPTIONS"
}

function teardown() {
	if [ -d "$COR_ROOT_DIR" ]
	then
		rm -r "$COR_ROOT_DIR"
	fi
}

@test "start without container id" {
	run $COR start
        [ "$status" -ne 0 ]
	[[ "${output}" == "Usage: start <container-id>" ]]
}

@test "start with invalid container id" {
	run $COR start
        [ "$status" -ne 0 ]
	[[ "${output}" == "Usage: start <container-id>" ]]
}

@test "start detach" {
	COR_TIMEOUT=5
	workload_cmd "sh"

	cmd="$COR create  --console --bundle $BUNDLE_DIR $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "created"

	cmd="$COR start $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "running"

	cmd="$COR attach $container_id"
	echo "exit" |  run_cmd "$cmd" "0" "$COR_TIMEOUT"
}

@test "start then stop" {
	COR_TIMEOUT=5
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

@test "start then kill (implicit signal)" {
	COR_TIMEOUT=5
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
	COR_TIMEOUT=5
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
	COR_TIMEOUT=5
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
	COR_TIMEOUT=5
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
