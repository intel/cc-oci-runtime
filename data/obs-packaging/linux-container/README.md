#linux-container

This directory contains the sources to update rpm specfiles and debian source
control files to generate ``cc-oci-runtime`` packages for Fedora and Ubuntu.

There is a script to generate specs and dsc automatically, to be pushed to
Open Build Service (http://openbuildservice.org/) and generate packages for you.


``./update_kernel.sh``

This scirpt will generate/update teh sources to create ``linux-container`` packages
  * linux-container.spec
  * lnux-container_X.X.XX-XX.dsc

By default it will generate the specs using the version specified in the file
``versions.txt`` at the the root of the repository
