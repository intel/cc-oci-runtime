# Intel® Clear Containers image

This source rpm contains the mini-os image needed to boot Clear Containers. 
To update and generate an rpm spec and dsc to create rpm and deb packages, 
you need to run:

``./update_image.sh``

By default, the script generates the specifications using the version
specified in the ``versions.txt`` file found at the the repository's root.
