.. contents::
.. sectnum::

Intel® Clear Containers OBS specification files
###############################################


The `Clear Containers`_ packages are available via `Open Build Service`_
to allow interested parties the opportunity to try out the technology even
if they are not using `Clear Linux` directly.

When the `cc-oci-runtime`_ or any of its components are updated, we release a
new package or set of packages.

Visit our repository at:

- http://download.opensuse.org/repositories/home:/clearlinux:/preview:/clear-containers-2.1/

Included components
===================

* `cc-oci-runtime`_: Includes the Clear Containers runtime.
* `clear-containers-image`_: Includes the mini-OS required to run Clear
  Containers.
* `clear-containers-selinux`_: Includes the SELinux policy module needed to
  run Clear Containers in environments with SELinux enabled.
* `linux-container`_: Includes the optimized guest kernel required to run Clear
  Containers
* `qemu-lite`_: Includes an optimized version of the QEMU hypervisor.


.. _`Clear Containers`:  https://clearlinux.org/features/intel%C2%AE-clear-containers

.. _`Clear Linux`: https://clearlinux.org

.. _`cc-oci-runtime`: https://github.com/01org/cc-oci-runtime

.. _`Open Build Service`: http://openbuildservice.org/

.. _`OBS`: http://openbuildservice.org/

.. _`qemu-lite`: https://github.com/01org/qemu-lite/tree/qemu-2.7-lite

.. _`linux-container`: https://www.kernel.org/

.. _`clear-containers-image`: https://download.clearlinux.org/current/

.. _`clear-containers-selinux`: https://github.com/gorozco1/cc-oci-runtime/tree/obs/proxy/selinux
