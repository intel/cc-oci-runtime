#!/bin/bash
# -*- mode: shell-script; indent-tabs-mode: nil; sh-basic-offset: 4; -*-
# ex: ts=8 sw=4 sts=4 et filetype=sh

# Automation script to create specs to build clear-containers-image
# Default image to build is the one specified in file versions.txt
# located at the root of the repository.
# set -x
AUTHOR=${AUTHOR:-$(git config user.name)}
AUTHOR_EMAIL=${AUTHOR_EMAIL:-$(git config user.email)}

CC_VERSIONS_FILE="../../../versions.txt"

source "$CC_VERSIONS_FILE"
VERSION=${1:-$clear_vm_image_version}

last_release=`cat wd/debian/changelog | head -1 | awk '{print $2}' | cut -d'-' -f2 | tr -d ')'`
next_release=$(( $last_release + 1 ))

OBS_PUSH=${OBS_PUSH:-false}

echo "Running: $0 $@"
echo "Update clear-containers-image to: $VERSION-$next_release"

function changelog_update {
    d=`date +"%a, %d %b %Y %H:%M:%S"`
    cp wd/debian/changelog wd/debian/changelog-bk
    cat <<< "clear-containers-image ($VERSION-$next_release) stable; urgency=medium

  * Update clear-containers-image $VERSION.

 -- $AUTHOR <$AUTHOR_EMAIL>  $d -0700
" > wd/debian/changelog
    cat wd/debian/changelog-bk >> wd/debian/changelog
    rm wd/debian/changelog-bk
}

changelog_update $VERSION

sed "s/\@VERSION\@/$VERSION/g; s/\@RELEASE\@/$next_release/g" clear-containers-image.spec-template > clear-containers-image.spec
sed "s/\@VERSION\@/$VERSION/g" wd/debian/rules-template > wd/debian/rules

chmod +x wd/debian/
spectool -g clear-containers-image.spec
tar -cJf clear-containers-image_$VERSION.orig.tar.xz clear-$VERSION-containers.img.xz LICENSE

cd wd
debuild -S -sa

if [ $? = 0 ] && [ "$OBS_PUSH" = true ]
then
    cd ..
    rm clear-containers-image_$VERSION-${next_release}_source.build \
    clear-containers-image_$VERSION-${next_release}_source.changes
    osc co home:clearlinux:preview:clear-containers-staging/clear-containers-image
    cd home\:clearlinux\:preview\:clear-containers-staging/clear-containers-image/
    osc rm clear-*-containers.img.xz
    osc rm clear-containers-image_*
    mv ../../clear-*-containers.img.xz  .
    mv ../../clear-containers-image_*  .
    mv ../../clear-containers-image.spec .
    cp ../../LICENSE .
    osc add clear-*-containers.img.xz
    osc add clear-containers-image_*
    osc commit -m "Update clear-containers-image to: $VERSION-$next_release"
fi
