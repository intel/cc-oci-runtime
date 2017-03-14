#!/bin/bash

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

#  Description of the test:
#  The objective of this tool is to report the memory map of a process in
#  a JSON file

declare MYSELF=${0##*/}
declare SAMPLES_FILE=$(mktemp)
declare DEFAULT_SAMPLES=10

declare NUM_SAMPLES
declare JSON_FILE
declare CC_ID

# shows help about this script
function help()
{
usage=$(cat << EOF
Usage: $MYSELF [-h] [--help] [-v] [--version]
  Description:
       This tool is to report the memory map of a process in
       a JSON file.
  Options:
       -h, --help             Help page
       -p, --process-name     Process Name
       -n, --num-samples      The number of samples that will be
                              collected
       -v, --version          Show version.
EOF
)
	echo "$usage";
}

# exit with error
function die()
{
	local msg="$*"
	echo >&2 "error: $msg"
	exit 1
}

# Checking env
function init()
{
	req_tools=(
		pmap
		awk
		bc)

	if [ -z "$PROC" ];then
		die "process name no found"
	fi

	if [ -z "$NUM_SAMPLES" ];then
		NUM_SAMPLES="$DEFAULT_SAMPLES"
	fi

	# check available tools
	echo "check required tools"
	for cmd in ${req_tools[*]};do
		command -v "$cmd" &> /dev/null
		res=$?
		if [ $res -ne 0 ];then
			die "command $cmd not available"
		fi
		echo "command: $cmd: yes"
	done
}

# dump memory avg to JSON file
function dump_avg_JSON()
{
	local pss=$1
	local shared=$2
	local private=$3
	local count=$4

	pss_avg=$(echo "scale=2; $pss / $count" | bc)
	shared_avg=$(echo "scale=2; $shared / $count" | bc -l )
	private_avg=$(echo "scale=2; $private / $count" | bc -l)

	echo -e "\t\t\"pss\": $pss_avg," >> $JSON_FILE
	echo -e "\t\t\"shared\": $shared_avg," >> $JSON_FILE
	echo -e "\t\t\"private\": $private_avg" >> $JSON_FILE
}

# it creates JSON file
function create_json()
{
	local process_name="$1"
	local samples_file="$2"

	local mapping=""
	local pss_mem=0
	local shared_mem=0
	local private_mem=0
	local count=0

	# accumulators
	local shared_sum=0
	local private_sum=0
	local pssmem_sum=0

	declare -A memory=(
		["mapping"]=1
		["pssmem"]=2
		["shared"]=3
		["private"]=4)

	JSON_FILE="${process_name}.json"
	output="$(cat "$samples_file" | sort)"

	# start JSON file creation
	echo "{" > "$JSON_FILE"

	while read line;do
		mapping=$(echo "$line" | awk -F "," -v column=${memory["mapping"]} '{print $column}')
		pss_mem=$(echo "$line" | awk -F "," -v column=${memory["pssmem"]} '{print $column}')
		shared_mem=$(echo "$line" | awk -F "," -v column=${memory["shared"]} '{print $column}')
		private_mem=$(echo "$line" | awk -F "," -v column=${memory["private"]} '{print $column}')
		count=$(( $count + 1 ))

		# last state
		if [ -z "$last_mapping" ];then
			last_mapping="$mapping"
		fi

		# dump data
		if [ "$mapping" != "$last_mapping" ];then
			echo -e "\t\"$last_mapping\": {" >> $JSON_FILE
			dump_avg_JSON $pssmem_sum $shared_sum $private_sum $count
			echo -e "\t}," >> $JSON_FILE
			shared_sum=0
			private_sum=0
			pssmem_sum=0
			count=0
			last_mapping="$mapping"
		fi

		# accumulate data
		shared_sum=$(( $shared_sum + $shared_mem ))
		private_sum=$(( $private_sum + $private_mem ))
		pssmem_sum=$(( $pssmem_sum + $pss_mem ))

	done <<< $(echo -e "$output")

	# dump buffers
	echo -e "\t\"$last_mapping\": {" >> $JSON_FILE
	dump_avg_JSON $pssmem_sum $shared_sum $private_sum $count
	echo -e "\t}" >> $JSON_FILE

	# Finish JSON file creation
	echo "}" >> $JSON_FILE
}

# take one sample of mapping
function take_map_sample()
{
	local ps_name="$1"

	# pmap tool shows a lot of
	# data fields, this number to ignore
	# head lines
	local format_columns=23

	# mapping tracking
	local last_mapping=""
	local mapping=""

	# memory values
	local pssmem=0
	local pssmem_sum=0
	local shared_memory=0
	local shared_clean=0
	local shared_dirty=0
	local shared_hugep=0
	local private_clean=0
	local private_dirty=0
	local private_hugep=0
	local private_memory=0

	# accumulators
	local shared_sum=0
	local private_sum=0
	local anon_pssmem=0
	local anon_shared=0
	local anon_private=0

	# Types of memory
	#
	# this array allows to filter:
	# - PSS memory
	# - (clean, dirty and hugetlb) shared memory
	# - (clean, dirty and hugetlb) private memory
	# form pmap tool output per mapping.
	declare -A memory=(
		["pssmem"]=8
		["sclean"]=9
		["sdirty"]=10
		["shugep"]=17
		["pclean"]=11
		["pdirty"]=12
		["phugep"]=18)

	# getting pid of the process
	pid=$(pidof $ps_name | awk '{print $1}')
	if [ -z "$pid" ];then
		docker rm -f  "$CC_ID"
		die "process does not exist"
	fi
	output=$(pmap $pid -XXpq)

	while read line;do
		mapping=$(echo "${line##* }" | grep "\/\|\[\|\]")
		line_columns=$(echo "$line" | wc -w)

		if (( $line_columns < $format_columns ));then
			continue
		fi

		head=$(echo "$line" | grep "$pid")
		if [ ! -z "$head" ];then
			continue
		fi

		# proportional set size memory
		pssmem=$(echo "$line" | awk -v column=${memory["pssmem"]} '{print $column}')

		# getting shared memory
		shared_clean=$(echo "$line" | awk -v column=${memory["sclean"]} '{print $column}')
		shared_dirty=$(echo "$line" | awk -v column=${memory["sdirty"]} '{print $column}')
		shared_hugep=$(echo "$line" | awk -v column=${memory["shugep"]} '{print $column}')

		# getting private memory
		private_clean=$(echo "$line" | awk -v column=${memory["pclean"]} '{print $column}')
		private_dirty=$(echo "$line" | awk -v column=${memory["pdirty"]} '{print $column}')
		private_hugep=$(echo "$line" | awk -v column=${memory["phugep"]} '{print $column}')

		# check for anon memory
		if [ -z "$mapping" ];then
			mapping="[anon]"
		fi

		# last state
		if [ -z "$last_mapping" ];then
			last_mapping="$mapping"
		fi

		# accumulate anon data
		if [ "$mapping" == "[anon]" ];then
			anon_pssmem=$(( $anon_pssmem + $pssmem ))
			anon_shared=$(( $anon_shared + $shared_memory))
			anon_private=$(( $anon_private + $private_memory ))
			continue
		fi

		# dump data
		if [ "$mapping" != "$last_mapping" ];then
			echo "$last_mapping,$pssmem_sum,$shared_sum,$private_sum" >> "$SAMPLES_FILE"
			shared_sum=0
			private_sum=0
			pssmem_sum=0
			last_mapping="$mapping"
		fi

		# calculate (shared and private) memory
		shared_memory=$(( $shared_clean + $shared_dirty + $shared_hugep))
		private_memory=$(( $private_clean + $private_dirty + $private_hugep))

		# accumulate data
		shared_sum=$(( $shared_sum + $shared_memory ))
		private_sum=$(( $private_sum + $private_memory ))
		pssmem_sum=$(( $pssmem_sum + $pssmem ))

	done <<< $(echo -e "$output")

	# dump buffer data"
	echo "$mapping,$pssmem_sum,$shared_sum,$private_sum" >> "$SAMPLES_FILE"
	echo "[anon],$anon_pssmem,$anon_shared,$anon_private" >> "$SAMPLES_FILE"
}

function main()
{
	while (( $# ));
	do
		case $1 in
		-h|--help)
			help
			exit 0;
		;;
		-p|--process-name)
			shift
			PROC="$1"
		;;
		-n|--num-samples)
			shift
			NUM_SAMPLES="$1"
		;;
		-v|--version)
			echo "$MYSELF version 0.1";
			exit 0;
		esac
		shift
	done

	init
	echo "Launching container"

	CC_ID=$(docker run -itd ubuntu bash)
	echo "cc id: $CC_ID"

	echo "Samples: "
	for (( i=0; i<$NUM_SAMPLES; i++ ));do
		take_map_sample "$PROC"
		printf "$i "
	done
	echo " Stop..."

	echo "Removing container"
	docker rm -f  "$CC_ID"

	echo "Creating JSON file"
	create_json "$PROC" "$SAMPLES_FILE"

	echo "Clean env"
	rm -f "$SAMPLES_FILE"

	echo "Finish"

	exit 0
}

main $@
