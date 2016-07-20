#!/usr/bin/env bats
#Based on runc commands
load common


@test "cor -h" {
  run $COR -h
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]
  [[ ${lines[1]} == *cc-oci-runtime* ]]

  run $COR --help
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]
  [[ ${lines[1]} == *cc-oci-runtime* ]]

  run $COR help
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]
  [[ ${lines[1]} == *cc-oci-runtime* ]]
}

@test "cor exec -h" {
  run "$COR" exec -h
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]

  run "$COR" exec --help
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]
}

@test "cor kill -h" {
  run "$COR" kill -h
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]

  run "$COR" kill --help
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]
}

@test "cor list -h" {

  run "$COR" list -h
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]

  run "$COR" list --help
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]
}

@test "cor pause -h" {

  run "$COR" pause -h
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]

  run "$COR" pause --help
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]
}

@test "cor resume -h" {

  run "$COR" resume -h
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]

  run "$COR" resume --help
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]
}

@test "cor start -h" {

  run "$COR" start -h
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]

  run "$COR" start --help
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]
}

@test "cor state -h" {

  run "$COR" state -h
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]

  run "$COR" state --help
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]
}

@test "cor delete -h" {

  run "$COR" delete -h
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]

  run "$COR" delete --help
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]
}

@test "cor stop -h" {

  run "$COR" stop -h
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]

  run "$COR" stop --help
  [ "$status" -eq 0 ]
  [[ ${lines[0]} =~ Usage:+ ]]
}

@test "cor foo -h" {
  run "$COR" foo -h
  [ "$status" -ne 0 ]
  [[ "${output}" == *"no such command: foo"* ]]

  run "$COR" foo --help
  [ "$status" -ne 0 ]
  [[ "${output}" == *"no such command: foo"* ]]
}
