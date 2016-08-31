#!/usr/bin/env bats
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
#

load common

@test "version" {
    run $COR -v
    [ "$status" -eq 0 ]
    [[ "${lines[0]}" =~ cc-oci-runtime\ version:\ [0-9.]+ ]]
    [[ "${lines[1]}" =~ spec\ version:\ [0-9.]+ ]]

    run $COR --version
    [ "$status" -eq 0 ]
    [[ "${lines[0]}" =~ cc-oci-runtime\ version:\ [0-9.]+ ]]
    [[ "${lines[1]}" =~ spec\ version:\ [0-9.]+ ]]
    [[ "${lines[2]}" =~ commit:\ [0-9a-f]+ ]]

    run $COR version
    [ "$status" -eq 0 ]
    [[ "${lines[0]}" =~ cc-oci-runtime\ version:\ [0-9.]+ ]]
    [[ "${lines[1]}" =~ spec\ version:\ [0-9.]+ ]]
    [[ "${lines[2]}" =~ commit:\ [0-9a-f]+ ]]
}
