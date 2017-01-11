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

set -e

# Perform go static tests.
#
#   .ci/ci-go-static-checks.sh [packages]
#
# One can specify the list of packages to test on the command line. If no #
# packages are specified, defaults to ./...

go_packages=$*

[ -z "$go_packages" ] && {
	go_packages=$(go list ./... | grep -v cc-oci-runtime/vendor |\
		    sed -e 's#.*/cc-oci-runtime/#./#')
}

function install_package {
	url="$1"
	name=${url##*/}

	echo Updating $name...
	go get -u $url
}

install_package github.com/fzipp/gocyclo
install_package github.com/client9/misspell/cmd/misspell
install_package github.com/golang/lint/golint

echo Doing go static checks on packages: $go_packages

go list -f '{{.Dir}}/*.go' $go_packages |\
    xargs -I % bash -c "misspell -error %"
go vet $go_packages
go list -f '{{.Dir}}' $go_packages |\
    xargs gofmt -s -l | wc -l |\
    xargs -I % bash -c "test % -eq 0"
go list -f '{{.Dir}}' $go_packages | xargs gocyclo -over 15

for p in $go_packages; do golint -set_exit_status $p; done
