#!/bin/bash
# Default kernel to build is the one specified in ../../../versions.txt
AUTHOR=${AUTHOR:-$(git config user.name)}
AUTHOR_EMAIL=${AUTHOR_EMAIL:-$(git config user.email)}
DEFAULT_VERSION=`cat ../../../versions.txt  | grep clear_vm_kernel_version | cut -d "=" -f 2 | cut -d'-' -f 1`
VERSION=${1:-$DEFAULT_VERSION}
echo "Packaging kernel $VERSION"
function changelog_update {
d=`date +"%a, %d %b %Y %H:%M:%S"`
last_release=`cat wd/debian/changelog | head -1 | awk '{print $2}' | cut -d'-' -f2 | tr -d ')'`
next_release=$(( $last_release + 1 ))
cp wd/debian/changelog wd/debian/changelog-bk
cat <<< "linux-container ($VERSION-$next_release) stable; urgency=medium

  * Update kernel to $VERSION

 -- $AUTHOR <$AUTHOR_EMAIL>  $d -0600
" > wd/debian/changelog
cat wd/debian/changelog-bk >> wd/debian/changelog
rm wd/debian/changelog-bk
}
changelog_update $VERSION
sed "s/\@VERSION\@/$VERSION/g; s/\@RELEASE\@/$next_release/g" linux-container.spec-template > linux-container.spec
spectool -g linux-container.spec
ln -s linux-$VERSION.tar.xz linux-container_$VERSION.orig.tar.xz
for i in `cat wd/debian/patches/series`
do
    cp $i wd/debian/patches/
done
cp config wd/debian/
cd wd
debuild -S -sa
rm debian/patches/*.patch
