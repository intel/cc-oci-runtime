#!/bin/bash
# $1 new version defaults to 2.1.4
#
# TODO
# Create an OBS Clear-Builder user to update cc-oci-runtime
AUTHOR=${AUTHOR:-$(git config user.name)}
AUTHOR_EMAIL=${AUTHOR_EMAIL:-$(git config user.email)}
DEFAULT_VERSION=`cat ../../../configure.ac  | grep cc-oci-runtime | grep AC_INIT | tr -d '[](),' | awk '{print $2}'
VERSION=${1:-$DEFAULT_VERSION}
echo "Packaging cc-oci-runtime $VERSION"
function changelog_update {
d=`date +"%a, %d %b %Y %H:%M:%S"`
hash_tag=`git log --oneline --pretty="%H %d" --decorate --tags --no-walk | grep $VERSION| awk '{print $1}'`
cp debian.changelog debian.changelog-bk
cat <<< "cc-oci-runtime ($VERSION) stable; urgency=medium

  * Update cc-oci-runtime to: $VERSION: ${hash_tag:0:7} 

 -- $AUTHOR <$AUTHOR_EMAIL>  $d -0600
" > debian.changelog
cat debian.changelog-bk >> debian.changelog
rm debian.changelog-bk
}
changelog_update $VERSION
sed "s/@VERSION@/$VERSION/g;" cc-oci-runtime.spec-template > cc-oci-runtime.spec
sed "s/@VERSION@/$VERSION/g;" cc-oci-runtime.dsc-template > cc-oci-runtime.dsc
sed "s/@VERSION@/$VERSION/g;" _service-template > _service
sed "s/@HASH_TAG@/$hash_tag/g;" update_commit_id.patch-template > update_commit_id.patch
