# Linux\* container kernel 

This source specifications contain the kernel needed to boot Clear
Containers. To update and generate an rpm spec and dsc to create rpm and deb
packages, you need to run:

``./update_kernel.sh``

By default, the script generates the specifications using the version
specified in the ``versions.txt`` file found at the the repository's root.
