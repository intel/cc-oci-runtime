.. contents::
.. sectnum::

``Clear Containers OBS spec files``
===================================


The `Clear Containers`_ packages are made available via `Open Build Service`_ to allow interested parties the opportunity to try out the technology, even if they are not using `Clear Linux` directly.

Once `cc-oci-runtime`_ or any of its components are updated, we release a new package or set of packages.

Repository:

- http://download.opensuse.org/repositories/home:/clearlinux:/preview:/clear-containers-2.1/

Components
----------

- `cc-oci-runtime`_: The Clear Containers runtime.
- `clear-containers-image`_: Contains the mini-OS required to run Clear
  Containers. 
- `clear-containers-selinux`_: SELinux policy module to run Clear Containers in
  environments with SELinux enabled.
- `linux-container`_: The optimised guest kernel required to run Clear
  Containers
- `qemu-lite`_: An optimised version of the QEMU hypervisor.


.. _`Clear Containers`:  https://clearlinux.org/features/intel%C2%AE-clear-containers

.. _`Clear Linux`: https://clearlinux.org

.. _`cc-oci-runtime`: https://github.com/01org/cc-oci-runtime

.. _`Open Build Service`: http://openbuildservice.org/

.. _`OBS`: http://openbuildservice.org/

.. _`qemu-lite`: https://github.com/01org/qemu-lite/tree/qemu-2.7-lite

.. _`linux-container`: https://www.kernel.org/ 

.. _`clear-containers-image`: https://download.clearlinux.org/current/

.. _`clear-containers-selinux`: https://github.com/gorozco1/cc-oci-runtime/tree/obs/proxy/selinux
