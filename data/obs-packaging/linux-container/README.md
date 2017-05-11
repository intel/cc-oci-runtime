# Linux\* container kernel

This directory contains the sources to update rpm specfiles and debian source
control files to generate ``linux-container``, the kernel needed to boot 
Intel® Clear Containers.

With these files we generated Fedora and Ubuntu packages for this component.

``./update_kernel.sh [VERSION]``

This script will generate and update sources to create ``linux-container``
packages.

  * linux-container.spec
  * lnux-container_X.X.XX-XX.dsc

By default, the script will get VERSION specified in the ``versions.txt`` file
found at the the repository's root.
