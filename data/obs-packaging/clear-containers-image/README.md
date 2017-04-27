#clear-containers image

This source rpm is the mini-os image used to boot clear-containers. 
To update and generate an rpm spec and dsc to create rpm and deb packages, 
you need to run:

``./update_image.sh``

By default it will generate the specs using the version specified in the file
``versions.txt`` at the the root of the repository
