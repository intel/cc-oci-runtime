.. contents::
.. sectnum::

Intel® Clear Containers OBS specification files
###############################################

The `Clear Containers`_ packages are available via `Open Build Service`_
to allow interested parties the opportunity to try out the technology even
if they are not using `Clear Linux` directly.

When the `cc-oci-runtime`_ or any of its components are updated, we release a
new package or set of packages.

This repo only contains the `OBS`_ sources to build the `cc-oci-runtime`_ and the remaining 
items are under: https://github.com/clearcontainers/packaging

Visit our repository at:

- http://download.opensuse.org/repositories/home:/clearlinux:/preview:/clear-containers-2.1/

Components needed to run Intel® Clear Containers 2.x
====================================================

* `cc-oci-runtime`_: The Clear Containers runtime.
* `clear-containers-image`_: The mini-OS required to run Clear
  Containers.
* `clear-containers-selinux`_: The SELinux policy module needed to
  run Clear Containers in environments with SELinux enabled.
* `kernel`_: The patches to build an optimized Linux kernel required to run Clear
  Containers
* `qemu-lite`_: The optimized version of the QEMU hypervisor.

.. _`Clear Containers`:  https://clearlinux.org/features/intel%C2%AE-clear-containers

.. _`Clear Linux`: https://clearlinux.org

.. _`cc-oci-runtime`: https://github.com/01org/cc-oci-runtime

.. _`Open Build Service`: http://openbuildservice.org/

.. _`OBS`: http://openbuildservice.org/

.. _`qemu-lite`: https://github.com/01org/qemu-lite/tree/qemu-2.7-lite

.. _`kernel`: https://github.com/clearcontainers/packaging/tree/master/kernel

.. _`clear-containers-image`: https://download.clearlinux.org/current/

.. _`clear-containers-selinux`: https://github.com/clearcontainers/proxy/tree/master/selinux
