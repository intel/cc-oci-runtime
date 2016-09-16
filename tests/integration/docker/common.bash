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

DOCKER_EXE="docker"
DOCKER_SERVICE="docker-cor"

#Cleaning test environment
function cleanDockerPs(){
	"$DOCKER_EXE" ps -q | xargs -r "$DOCKER_EXE" kill
	"$DOCKER_EXE" ps -aq | xargs -r "$DOCKER_EXE" rm -f
}

#Restarting test environment
function startDockerService(){
	systemctl status "$DOCKER_SERVICE" | grep 'running'
	if [ "$?" -eq 0 ]; then
		systemctl restart "$DOCKER_SERVICE"
	fi
}

#Checking that default runtime is cor
function runtimeDocker(){
    default_runtime=`$DOCKER_EXE info 2>/dev/null | grep "^Default Runtime" | cut -d: -f2 | tr -d '[[:space:]]'`
    if [ "$default_runtime" != "cor" ]; then
        skip "Tests need to run with cor as default runtime"
    fi
}
