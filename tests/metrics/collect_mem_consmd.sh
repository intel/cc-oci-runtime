#!/bin/bash

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

#  Description of the test:
#  The objective of this tool is to measure the average memory consumption
#  of a Clear Containers component during a determined Clear Containers
#  stress flow.
#
#  Note: It is recommended to launch the same number of Clear Containers per
#  execution in order to compare results.

# set -x
set -e

# General env
MYSELF=${0##*/}
CC_NUM=5
MSGS_NUM=10000
MSG_SIZE=80
SHARED_DIR="shared"
PROC="cc-proxy"

# Host/Container paths
HOST_SHARED_PATH="$(pwd)/$SHARED_DIR"
CONT_SHARED_PATH="/root/$SHARED_DIR"

# Aux files
HOST_GATE="$HOST_SHARED_PATH/gate"
CONT_GATE="$CONT_SHARED_PATH/gate"
HOST_RDM_FILE="$HOST_SHARED_PATH/out.data"
CONT_RDM_FILE="$CONT_SHARED_PATH/out.data"
CSV_FILE="mem.csv"
PSS_FILE="pss.log"

declare WORKLOAD
declare COLLECT_PID
declare SMEM_PID
declare F_OPTS
declare DATA_RES
declare RSS_RES
declare PSS_RES

# columns in collect output
declare -A COLLECT_MEM=(
	["VmStk"]=13
	["VmData"]=12
	["VmRSS"]=11
	["VmLck"]=10)

# Shows help about this script
function help()
{
usage=$(cat << EOF
Usage: $MYSELF [-h] [--help] [-v] [--version]
  Description:
       This tool takes memory consumption
       information about a determined number
       of containers.
  Options:
       -h, --help             Help page
       -c, --num-containers   Set the number of containers
                              that will be launched.
       -m, --num-messages     The number of messages is the
                              workload of each container.
       -s, --size-message     The number of characters per
                              message.
       -p, --process-filter   This option restricts which
                              processes are selected for
			      collection/display tool.
       -x --exact-match       Get metrics just about exact
                              match.
       -v, --version          Show version.
EOF
)
	echo "$usage";
}

# This workload will run inside of each Clear
# Container, it will monitor a shared path with
# host, this is in order to establish communication
# with host and activate the I/O read/write
# workload at the same time in all containers.
function implement_workload()
{
	WORKLOAD=$(cat << EOF
	#!/bin/bash

	function print_rdmsg()
	{
		echo "$(cat /dev/urandom 2>/dev/null | \
			LC_CTYPE=C tr -dc "[:alnum:]" 2>/dev/null | \
			head -c $MSG_SIZE )"
	}

	while [ 1 ];do
		if [ -f "$CONT_GATE" ];then
			for (( i=0; i<$MSGS_NUM; i++ ));do
				print_rdmsg
			done
			break;
		else
			sleep 0.2;
		fi
	done
	exit 0;
EOF
)
}

# This function launch collectl tool
# in order to get metrics about a specific tool.
function start_collect()
{
	echo "Launching collectl"
	collectl -oTm -sZm -i0.5:1 \
		--procfilt f$PROC \
		--procopts tm -f $PROC-mem &

	COLLECT_PID=$!
	echo "collectl pid: $COLLECT_PID"
}

# It kills the collectl process.
function stop_collect()
{
	echo "Stop collectl"
	kill "$COLLECT_PID"
}

# It launches a instance of smem tool
# in order to get metrics about proportional
# set size.
function start_pss_monitor()
{
	bash ./smem_monitor.sh $PROC &
	SMEM_PID=$!
}

# It terms smem process
function stop_pss_monitor()
{
	kill "$SMEM_PID"
}

# Get PSS memory metrics
function get_pss_metrics()
{
	pss_sum=0
	pss_count=0
	samples=""

	while read line; do
		pss_mem=$(echo "$line" | sed 's/[A-Za-z]*//g')
		res=$(echo "$pss_mem > 0" | bc )
		if [ $res -eq 1 ];then
			pss_sum=$(echo "$pss_sum + $pss_mem"| bc -l )
			pss_count=$(( $pss_count + 1 ))
			if [ ! -z "$samples" ];then
				samples="$samples ,"
			fi
			samples="$samples $pss_mem"
		fi
	done < "$PSS_FILE"

	stdev=$(python3 -c "import statistics; print (statistics.stdev([$samples]))")
	avg=$(echo "$pss_sum / $pss_count" | bc -l)
	pstdev=$(echo "($stdev / $avg) * 100" | bc -l)
	echo "avg: $avg stdev: $stdev pstdev: $pstdev"
}

# Get metrics about a "csv" file produced
# by collectl tool.
function get_collect_metrics()
{
	data_sum=0
	data_count=0
	samples=""

	while read line; do
		cmd=$(echo "$line" | awk -F "," '{print $30}' | grep $PROC)
		if [ -z "$cmd" ];then
			continue
		fi

		binary="$(echo "$cmd" | cut -d " " -f1 | \
			xargs basename -z | grep $F_OPTS $PROC)"

		if [ -z "$binary" ];then
			continue
		fi

		data_mem=$(echo "$line" | awk -v column=$1 -F "," '{print $column}')

		if (( $data_mem > 0 ));then
			data_sum=$(( $data_sum + $data_mem ))
			data_count=$(( $data_count + 1 ))
			if [ -n "$samples" ];then
				samples="$samples ,"
			fi
			samples="$samples $data_mem"
		fi

	done < "$CSV_FILE"

		if (( $data_count == 0 ));then
		echo -e "\e[31m$MYSELF: error: no collected samples\e[39m"
		exit 1;
	fi

	stdev=$(python3 -c "import statistics; print(statistics.stdev([$samples]))")
	avg=$(echo "$data_sum / $data_count" | bc -l)
	pstdev=$(echo "($stdev / $avg) * 100" | bc -l)

	echo "avg: $avg stdev: $stdev pstdev: $pstdev"
}

# Print the metrics memory
function print_results()
{
	echo -e "\e[33m==== Results ===="
	echo "$PROC VmData: $DATA_RES"
	echo "$PROC RSS:    $RSS_RES"
	echo "$PROC PSS:    $PSS_RES"
	echo -e "=================\e[39m"
}

# It creates a "csv" file using a raw information
# file created by collectl tool.
function create_csv_file()
{
	raw_file=$(ls | grep *.gz | head -n1)
	collectl -sZ -oT -P --sep ',' -p "$raw_file" > "$CSV_FILE"
}

# cc-proxy binary is launched by a systemd
# service (it is possible to launches it from cmd),
# if that service is not running, it is
# not possible to boot a Clear Container, this
# function verify that.
function check_proxy_service()
{
	status=$(systemctl status cc-proxy | grep -ow active)

	if [[ $status == "active" ]];then
		echo "cc-proxy is running"
		echo "restart cc-proxy"
		systemctl restart cc-proxy.service
	else
		echo "cc-proxy is not running"
		echo "start cc-proxy"
		systemctl start cc-proxy.service
	fi

	if [ $? -ne 0 ];then
		echo -e "\e[31merror: cc-proxy could not start\e[39m"
		exit 1
	fi
}

function clean_docker_waste()
{
	echo "Cleaning environment"

	# clean docker waste
	waste="$(docker ps -qa)"
	if [ ! -z "$waste" ];then
		docker rm -f $waste
	fi
}

# Exit with error
function die()
{
	local msg="$*"
	echo >&2 "error: $msg"
	exit 1
}

# This function will check the necessary requirements
# in the environment for running Clear Containers.
function set_environment()
{
	req_tools=(
		collectl
		smem
		bc
		python3)

	# check available tools
	echo "Check required tools"
	for cmd in ${req_tools[*]};do
		command -v "$cmd" &>/dev/null
		res=$?
		if [ $res -eq 0 ];then
			echo "command: $cmd: yes"
			continue
		fi

		die "command $cmd not available"
	done

	clean_docker_waste

	# clean result files
	rm -rf *.gz
	rm -rf *.csv
	rm -rf *.log

	if [ -d "$HOST_SHARED_PATH" ];then
		echo "Cleaning shared directory"
		rm -rf "$HOST_SHARED_PATH"
	fi

	mkdir -p $HOST_SHARED_PATH

	echo "Restart docker service"
	systemctl restart docker-cor.service
}

# main function
function main()
{
	while (( $# ));
	do
		case $1 in
		-h|--help)
			help
			exit 0;
		;;
		-c|--num-containers)
			shift
			CC_NUM=$1
		;;
		-m|--num-messages)
			shift
			MSGS_NUM=$1
		;;
		-s|--size-message)
			shift
			MSG_SIZE=$1
		;;
		-p|--process-filter)
			shift
			PROC=$1
		;;
                -x|--exact-match)
			F_OPTS="-wx"
		;;
		-v|--version)
			echo "$MYSELF version 0.1";
			exit 0;
		esac
		shift
	done

	set_environment
	implement_workload

	echo "Stop cc-proxy"
	systemctl stop cc-proxy
	start_collect
	sleep 0.5
	check_proxy_service

	for (( i=0; i<$CC_NUM; i++ ))
	do
		docker run -v $HOST_SHARED_PATH:$CONT_SHARED_PATH \
			-d -i -t  ubuntu bash -c "$WORKLOAD"
	done

	echo "Active gate"
	echo "True" > $HOST_GATE
	start_pss_monitor

	while [ 1 ];
	do
		cont_num=$(docker ps | wc -l)
		if (( $cont_num == 1 ));then
			stop_pss_monitor
			break
		fi

		sleep 0.1
	done

	echo "Workload finishes"
	stop_collect
	create_csv_file

	echo "Making a total"
	RSS_RES=$(get_collect_metrics ${COLLECT_MEM["VmRSS"]})
	DATA_RES=$(get_collect_metrics ${COLLECT_MEM["VmData"]})
	PSS_RES=$(get_pss_metrics)

	clean_docker_waste
	print_results
	echo -e "\e[32mFinish\e[39m"

	exit 0;
}

# call to main function
main $@
