#cc-oci-runtime

This directory contains the sources to update rpm specfiles and debian source
control files to generate ``cc-oci-runtime`` packages for Fedora and Ubuntu.

There is a script to generate specs and dsc automatically, to be pushed to
Open Build Service (http://openbuildservice.org/) and generate packages for you.


``./update_runtime.sh``

This script will generate/update the sources to create ``cc-oci-runtime`` packages.
  * cc-oci-runtime.dsc
  * cc-oci-runtime.spec


By default it will generate the specs using the version specified in the file
``configure.ac`` at the the root of the repository
