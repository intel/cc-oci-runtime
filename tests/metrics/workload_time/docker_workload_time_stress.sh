#!/bin/bash

#  This file is part of cc-oci-runtime.
#
#  Copyright (C) 2016-2017 Intel Corporation
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

#  Description of the test:
#  This test measures the complete workload of a container using docker.
#  From calling docker until the workload is completed and the container
#  is shutdown.

[ $# -ne 3 ] && die "Usage: $0 <cmd to run> <image> <runtime>"

SCRIPT_PATH=$(dirname "$(readlink -f "$0")")
source "${SCRIPT_PATH}/../../lib/test-common.bash"

CMD="$1"
IMAGE="$2"
RUNTIME="$3"
TMP_FILE=$(mktemp workloadTime.XXXXXXXXXX)
TEST_NAME="docker run time"
TEST_ARGS="image=${IMAGE} command=${CMD} runtime=${RUNTIME}"

# Get the time it takes to run a given workload
function run_workload(){
    [ -n "$1" ] || die "parameter not set"
    TEST_RESULT_FILE=$(echo "${RESULT_DIR}/${TEST_NAME}-${IMAGE}-${CMD}-${RUNTIME}_$1" | sed 's| |-|g')
    backup_old_file "$TEST_RESULT_FILE"
    write_csv_header "$TEST_RESULT_FILE"

    if [[ "$RUNTIME" != 'runc' && "$RUNTIME" != 'cor' ]]; then
	die "Runtime ${RUNTIME} is not valid"
    fi
    (eval time -p "$DOCKER_EXE" run --rm -ti --runtime "$RUNTIME" "$IMAGE" "$CMD") &> "$TMP_FILE"
    if [ $? -eq 0 ]; then
	test_data=$(grep ^real "$TMP_FILE" | cut -f2 -d' ')
	write_result_to_file "$TEST_NAME" "$TEST_ARGS" "$test_data" "$TEST_RESULT_FILE"
    fi
    rm -f "$TMP_FILE"
}

echo "Executing test: ${TEST_NAME} ${TEST_ARGS}"
for x in 10 50 100
do
    printf "\n($x) loop => "
    for ((y=1; y<=$x; y++))
    do
	printf "$y "
	docker run -tid ubuntu bash >/dev/null
    done
    run_workload $x
    docker rm -f $(docker ps -aq) >/dev/null
done
