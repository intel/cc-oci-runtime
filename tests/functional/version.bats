#!/usr/bin/env bats

load common

@test "version" {
    run $COR -v
    [ "$status" -eq 0 ]
    [[ "${lines[0]}" =~ clr-oci-runtime\ version:\ [0-9.]+ ]]
    [[ "${lines[1]}" =~ spec\ version:\ [0-9.]+ ]]

    run $COR --version
    [ "$status" -eq 0 ]
    [[ "${lines[0]}" =~ clr-oci-runtime\ version:\ [0-9.]+ ]]
    [[ "${lines[1]}" =~ spec\ version:\ [0-9.]+ ]]
    [[ "${lines[2]}" =~ commit:\ [0-9a-f]+ ]]

    run $COR version
    [ "$status" -eq 0 ]
    [[ "${lines[0]}" =~ clr-oci-runtime\ version:\ [0-9.]+ ]]
    [[ "${lines[1]}" =~ spec\ version:\ [0-9.]+ ]]
    [[ "${lines[2]}" =~ commit:\ [0-9a-f]+ ]]
}
