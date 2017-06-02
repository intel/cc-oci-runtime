#!/bin/bash
# -*- mode: shell-script; indent-tabs-mode: nil; sh-basic-offset: 4; -*-
# ex: ts=8 sw=4 sts=4 et filetype=sh

# Automation script to create specs to build clear-containers-image
# Default image to build is the one specified in file versions.txt
# located at the root of the repository.
set -x
AUTHOR=${AUTHOR:-$(git config user.name)}
AUTHOR_EMAIL=${AUTHOR_EMAIL:-$(git config user.email)}

CC_VERSIONS_FILE="../../../versions.txt"
source "$CC_VERSIONS_FILE"
VERSION=${1:-$clear_vm_image_version}

OBS_PUSH=${OBS_PUSH:-false}
OBS_CC_IMAGE_REPO=${OBS_RUNTIME_REPO:-home:clearlinux:preview:clear-containers-staging/clear-containers-image}

git checkout wd/debian/changelog
last_release=`cat wd/debian/changelog | head -1 | awk '{print $2}' | cut -d'-' -f2 | tr -d ')'`
next_release=$(( $last_release + 1 ))

echo "Running: $0 $@"
echo "Update clear-containers-image to: $VERSION-$next_release"

function changelog_update {
    d=$(date +"%a, %d %b %Y %H:%M:%S %z")
    cp wd/debian/changelog wd/debian/changelog-bk
    cat <<< "clear-containers-image ($VERSION-$next_release) stable; urgency=medium

  * Update clear-containers-image $VERSION.

 -- $AUTHOR <$AUTHOR_EMAIL>  $d
" > wd/debian/changelog
    cat wd/debian/changelog-bk >> wd/debian/changelog
    rm wd/debian/changelog-bk
}

changelog_update $VERSION

sed "s/\@VERSION\@/$VERSION/g; s/\@RELEASE\@/$next_release/g" clear-containers-image.spec-template > clear-containers-image.spec
sed "s/\@VERSION\@/$VERSION/g" wd/debian/rules-template > wd/debian/rules

chmod +x wd/debian/rules
spectool -g clear-containers-image.spec
tar -cJf clear-containers-image_$VERSION.orig.tar.xz clear-$VERSION-containers.img.xz LICENSE

cd wd
debuild -S -sa

if [ $? = 0 ] && [ "$OBS_PUSH" = true ]
then
    temp=$(basename $0)
    TMPDIR=$(mktemp -d -t ${temp}.XXXXXXXXXXX) || exit 1
    cd ..
    cc_image_dir=$(pwd)
    rm clear-containers-image_$VERSION-${next_release}_source.build \
    clear-containers-image_$VERSION-${next_release}_source.changes
    osc co "$OBS_CC_IMAGE_REPO" -o $TMPDIR
    cd $TMPDIR
    osc rm clear-*-containers.img.xz
    osc rm clear-containers-image_*
    mv $cc_image_dir/clear-*-containers.img.xz  .
    mv $cc_image_dir/clear-containers-image_*  .
    mv $cc_image_dir/clear-containers-image.spec .
    cp $cc_image_dir/LICENSE .
    osc add clear-*-containers.img.xz
    osc add clear-containers-image_*
    osc commit -m "Update clear-containers-image to: $VERSION-$next_release"
fi
