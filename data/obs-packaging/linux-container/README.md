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

Open Build Service
------------------

The script has two OBS related variables. Using them, the CI can push changes
to the [OBS website] (https://build.opensuse.org/).

  * ``OBS_PUSH`` default ``false``
  * ``OBS_CC_KERNEL_REPO`` default ``home:clearlinux:preview:clear-containers-staging/linux-container``

To push your changes and trigger a new build of the runtime to the OBS repo,
set the variables in the environment running the script before calling
``update_runtime.sh`` as follows:

```bash
export OBS_PUSH=true
export OBS_CC_KERNEL_REPO=home:patux:clear-containers-2.1/linux-container

./update_kernel.sh [VERSION]
```
