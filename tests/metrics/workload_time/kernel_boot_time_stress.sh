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
#  This test measures the time kernelspace takes when a clear container
#  boots.

SCRIPT_PATH=$(dirname "$(readlink -f "$0")")
source "${SCRIPT_PATH}/../../lib/test-common.bash"

TEST_NAME="Stress Kernel Boot Time"
TMP_FILE=$(mktemp dmesglog.XXXXXXXXXX)

# Get the time it takes to boot a container
function get_kboot_time() {
    [ -n "$1" ] || die "parameter not set"
    test_result_file=$(echo "${RESULT_DIR}/${TEST_NAME}_$1" | sed 's| |-|g')
    backup_old_file "$test_result_file"
    write_csv_header "$test_result_file"
    eval docker run -ti clearlinux dmesg > "$TMP_FILE"
    test_data=$(grep "Freeing" "$TMP_FILE" | tail -1 | awk '{print $2}' | cut -d']' -f1)
    write_result_to_file "$TEST_NAME" "$test_args" "$test_data" "$test_result_file"
}

function main_loop(){
    for x in 10 50 100
    do
	printf "\n($x) loop => "
	for ((y=1; y<=$x; y++))
	do
	    printf "$y "
	    docker run -tid ubuntu bash >/dev/null
	done
	get_kboot_time $x
	docker rm -f $(docker ps -aq) >/dev/null
    done
    rm "$TMP_FILE"
}

echo "Executing test: ${TEST_NAME}"
main_loop
