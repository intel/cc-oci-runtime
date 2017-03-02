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
#  The objective of this tool is to measure the PSS average memory.

set -e


declare PROC=$1
declare PSSLOG="pss.log"

trap "echo stop to monitor;exit 0;" SIGINT SIGTERM

if [ -z "$PROC" ];then
	echo "error: usage: $0 [process name]"
	exit 1;
fi

while [ 1 ];do
	mem_sum=0
	count=0

	# $6 is PSS colum in smem output
	data=$(smem --no-header -P "^$PROC" | awk '{print $6}')
	for i in $data;do
		if (( $i > 0 ));then
			mem_sum=$(( $i + $mem_sum ))
			count=$(( $count + 1 ))
		fi
	done

	if (( $count > 0 ));then
		avg=0
		avg=$(echo "$mem_sum / $count" |  bc -l)
		echo "$avg" >> "$PSSLOG"
	fi

	sleep 0.2
done
