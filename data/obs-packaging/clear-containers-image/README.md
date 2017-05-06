#clear-containers-image

This directory contains the sources to update rpm specfiles and debian source
control files to generate ``clear-containers-image`` packages for Fedora and Ubuntu.

There is a script to generate specs and dsc automatically, to be pushed to
Open Build Service (http://openbuildservice.org/) and generate packages for you.

``./update_image.sh``

This script will generate/update:
  * clear-containers-image_XXXX-XX.dsc
  * clear-containers-image.spec


By default it will generate the specs using the version specified in the file
``versions.txt`` at the the root of the repository

