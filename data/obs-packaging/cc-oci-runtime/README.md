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

Open Build Service
------------------

The script has two OBS related variables. Using them, the CI can push changes
to the [OBS website] (https://build.opensuse.org/).

The variables with their default values are as follows:

  * ``OBS_PUSH`` default ``false``
  * ``OBS_RUNTIME_REPO`` default ``home:clearlinux:preview:clear-containers-staging/cc-oci-runtime``

To push your changes and trigger a new build of the runtime to the OBS repo,
set the variables in the environment running the script before calling
``update_runtime.sh`` as follows:

```bash
export OBS_PUSH=true
export OBS_RUNTIME_REPO=home:patux:clear-containers-2.1/cc-oci-runtime

./update_runtime.sh [VERSION]
```
