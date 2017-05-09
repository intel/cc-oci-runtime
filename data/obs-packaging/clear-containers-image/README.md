# Intel® Clear Containers image

This directory contains the sources to update rpm specfiles and debian source
control files to generate ``clear-containers-image`` the mini-os image needed
to boot Intel® Clear Containers.

With these files we generated fedora and ubuntu packages for this component.

``./update_image.sh [VERSION]``

This script will generate/update:

  * clear-containers-image_XXXX-XX.dsc
  * clear-containers-image.spec

By default, the script will get VERSION specified in the ``versions.txt`` file
found at the the repository's root.
