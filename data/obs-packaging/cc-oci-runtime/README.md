# cc-oci-runtime

This directory contains the sources to create rpm specfiles and debian source
control files to create ``cc-oci-runtime`` The runtime of Intel® Clear 
Containers.

With these files we generated Fedora and Ubuntu packages for this component.

``./update_runtime.sh [VERSION]``

This script will update the sources to create ``cc-oci-runtime`` packages.

  * cc-oci-runtime.dsc
  * cc-oci-runtime.spec

By default, the script will get VERSION specified in the ``versions.txt`` file
found at the the repository's root.
