#!/bin/bash
# Default image to build is the one specified in ../../../versions.txt
AUTHOR=${AUTHOR:-$(git config user.name)}
AUTHOR_EMAIL=${AUTHOR_EMAIL:-$(git config user.email)}
DEFAULT_VERSION=`cat ../../../versions.txt  | grep clear_vm_image_version | cut -d "=" -f 2`
VERSION=${1:-$DEFAULT_VERSION}
function changelog_update {
d=`date +"%a, %d %b %Y %H:%M:%S"`
last_release=`cat wd/debian/changelog | head -1 | awk '{print $2}' | cut -d'-' -f2 | tr -d ')'`
next_release=$(( $last_release + 1 ))
cp wd/debian/changelog wd/debian/changelog-bk
cat <<< "clear-containers-image ($VERSION-$next_release) stable; urgency=medium

  * Update to version $VERSION.

 -- $AUTHOR <$AUTHOR_EMAIL>  $d -0700
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
