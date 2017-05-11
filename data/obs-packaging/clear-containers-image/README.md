# Intel® Clear Containers image

This directory contains the sources to update rpm specfiles anddebian source
control files to generate ``clear-containers-image`` the mini-os image needed
to boot Intel® Clear Containers.

With these files we generated Fedora and Ubuntu packages for this component.

``./update_image.sh [VERSION]``

This script will generate and update sources to create
``clear-containers-image`` packages.

  * clear-containers-image_XXXX-XX.dsc
  * clear-containers-image.spec

By default, the script will get VERSION specified in the ``versions.txt`` file
found at the the repository's root.
