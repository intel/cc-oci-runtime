#!/usr/bin/env bats

load common

function setup() {
	setup_common
	check_ccontainers
	COR_TIMEOUT=5
	container_id="tests_id"
}

function teardown() {
	cleanup_common
}



# Run and vefiy state output
# @param <container-id>
# @param <status> container state to vefiry (running, created ...)
function check_state() {
	container_id_state="$1"
	status="$2"
	#COR must store resolved paths to cc components
	cc_kernel_path=$(readlink -e "${CONTAINER_KERNEL}")
	cc_image_path=$(readlink -e "${CONTAINERS_IMG}")

	state_cmd="$COR state $container_id_state"
	echo $state_cmd >&2
	output=$(run_cmd "$state_cmd" "0" "$COR_TIMEOUT")
	echo "$output" | grep "\"id\" : \"$container_id_state\""
	echo "$output" | grep "\"bundlePath\" : \"$BUNDLE_DIR\""
	echo "$output" | grep "\"status\" : \"$status\""
	echo "$output" | grep "\"hypervisor_path\" : \"$HYPERVISOR_PATH\""
	echo "$output" | grep "\"image_path\" : \"$cc_image_path\""
	echo "$output" | grep "\"kernel_path\" : \"$cc_kernel_path\""
}

@test "start and kill state" {
	workload_cmd "sh"

	cmd="$COR create  --console --bundle $BUNDLE_DIR $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "created"
	check_state "$container_id" "created"


	cmd="$COR start $container_id"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "running"
	check_state "$container_id" "running"

	cmd="$COR kill $container_id 15"
	run_cmd "$cmd" "0" "$COR_TIMEOUT"
	testcontainer "$container_id" "stopped"
	check_state "$container_id" "stopped"
}

@test "state not existing container" {

	state_cmd="$COR state $container_id"
	output=$(run_cmd "$state_cmd" "1" "$COR_TIMEOUT")
	echo $output | grep "failed to read state file"
}
