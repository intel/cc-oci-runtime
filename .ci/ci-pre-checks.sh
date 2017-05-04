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

#---------------------------------------------------------------------
# Description: Perform basic checks on the branch before attempting
#   to run the build and test suites. If this script fails, the CI run
#   should be aborted.
#---------------------------------------------------------------------

typeset script_name=${0##*/}

# Limits defined in CONTRIBUTING.md
typeset -i max_subject_length=75
typeset -i max_body_line_length=72

# Latest commit (HEAD) in current branch
typeset commit

# "Destination" git branch that the new commit(s) in the
# current branch want to be merged into.
typeset branch

# Set to 'yes' when a commit is found that specifies an
# issue this PR fixes.
typeset found_fixes=no

# Log a message to stderr and exit
die()
{
    msg="$*"
    echo >&2 -e "ERROR: $script_name: commit $commit: $msg"
    exit 1
}

# Log an informational string to stdout
msg()
{
    str="$*"
    echo -e "INFO: $script_name: $str"
}

# Perform checks on git subject line
check_commit_subject()
{
    local subject="$1"

    [ -n "$subject" ] || die "no subject"

    echo "$subject"|grep -q "^[^ ][^ ]*.*:" \
        || die "no subsystem in subject"

    len=$(echo -n "$subject"|wc -c)

    [ "$len" -gt "$max_subject_length" ] \
        && die "subject too long (length $len, max $max_subject_length)"
}

# Perform checks on git subject line
check_commit_body()
{
    local body="$1"

    local -a lines

    [ -n "$body" ] || die "no body"

    # Load the body text into an array
    mapfile -t lines < <( echo "$body" )

    for line in "${lines[@]}"
    do
        echo "$line"|egrep -iq "\<fixes\>:*[ ]*#[0-9][0-9]*"
        if [ $? -eq 0 ]
        then
            found_fixes=yes
        fi

        len=$(echo -n "$line"|wc -c)

        [ "$len" -gt "$max_body_line_length" ] \
            && die "body line too long (length $len, max $max_body_line_length):\n\n  '$line'"
    done
}

# Perform checks on a single commit
check_commit()
{
    local commit="$1"

    msg "Checking commit $commit"

    local subject=$(git log -1 --pretty="%s" "$commit")
    local body=$(git log -1 --pretty="%b" "$commit")

    check_commit_subject "$subject"
    check_commit_body "$body"
}

# Perform checks on specified list of commits
check_commits()
{
    commits="$*"

    for commit in $commits
    do
        check_commit "$commit"
    done

    [ $found_fixes = yes ] || die "no 'Fixes #XXX' comment"
}

# Determine the git commit and branch from the CI environment.
if [ "$TRAVIS" = true ]
then
    commit="$TRAVIS_COMMIT"
    branch="$TRAVIS_BRANCH"
elif [ "$SEMAPHORE" = true ]
then
    commit="$REVISION"
    branch="$BRANCH_NAME"
else
    die "unknown CI system"
fi

[ -n "$commit" ] || die "no commit"
[ -n "$branch" ] || die "no branch"

msg "target branch: $branch"
msg "top commit in current branch: $commit"

# List of commits in current branch that are not in the target branch
# Order is oldest first (since that is the natural order in which to
# check the commits).
commits=$(git rev-list --no-merges --reverse "${branch}..")

count=$(echo "$commits"|wc -l)
msg "Commits to check: $count"

check_commits "$commits"

exit 0
