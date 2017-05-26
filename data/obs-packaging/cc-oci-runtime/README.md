# cc-oci-runtime

This directory contains the sources to create rpm specfiles and debian source
control files to create ``cc-oci-runtime`` The runtime of Intel® Clear 
Containers.

With these files we generated Fedora and Ubuntu packages for this component.

``./update_runtime.sh [VERSION]``

The ``VERSION`` parameter is optional. The parameter can be a tag, a branch,
or a GIT hash.

If the ``VERSION`` parameter is not specified, the top-level ``configure.ac``
file will determine its value automatically.

This script will update the sources to create ``cc-oci-runtime`` packages.

  * cc-oci-runtime.dsc
  * cc-oci-runtime.spec
