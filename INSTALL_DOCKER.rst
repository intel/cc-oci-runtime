.. contents::
.. sectnum::

``Installing Clear Containers 2.0 into Clear Linux``
====================================================

Introduction
------------
`Clear Containers`_ 2.0 provides an Open Containers Initiative (OCI_) compatible 'runtime' and is installable into Docker_ 1.12.0 and later, where OCI_ runtime support is available.

You will need to have a `Clear Linux`_ installation before commencing this procedure, although `Clear Containers`_ do not depend on `Clear Linux`_ as a host and can be run on top of other distributions. See `Installing Clear Linux`_ for more details.


Overview
--------
The following steps install and configure `Clear Containers`_ and Docker_ into an existing `Clear Linux`_ distribution. You will require `Clear Linux`_ version 9890 or above.

Again, please note that `Clear Containers`_ can run on top of other distributions. Here, `Clear Linux`_ is used as one example of a distribution `Clear Containers`_ can run on. This document does not cover installing `Clear Containers`_ on other distributions.

After this installation you will be able to launch Docker_ container payloads using either the default Docker_ (``runc``) Linux Container runtime or the `Clear Containers`_ QEMU/KVM hypervisor based runtime (``cc-oci-runtime``).


Installation Steps
------------------

Enable sudo
~~~~~~~~~~~

You will need root privileges in order to run a number of the following commands. It is recommended you run these commands from a user account with ``sudo`` rights. 

If your user does not already have ``sudo`` rights, you should add your user to ``wheel`` group whilst logged in as ``root``:

  ::

    # usermod -G wheel -a <USERNAME>

This change will take effect once you have logged out as root and logged back in as <USERNAME>.

And you will also need to add your user or group to the ``/etc/sudoers`` file, for example:

  ::

    # visudo
    #and add the line:
      %wheel  ALL=(ALL)    ALL

You can now log out of ``root``, log in as your <USERNAME> and continue by using ``sudo``.

Check Clear Container compatability
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Before you try to install and run `Clear Containers`_ it is prudent to check that your machine (hardware) is compatible. The easiest way to do this is to download and run the Clear Containers check config script:

  ::

    $ curl -O https://download.clearlinux.org/current/clear-linux-check-config.sh
    $ chmod +x clear-linux-check-config.sh
    $ ./clear-linux-check-config.sh container

This command will print a list of test results. All items should return a 'SUCCESS' status - but you can ignore the 'Nested KVM support' item if it fails - this just means you cannot run `Clear Containers`_ under another hypervisor such as KVM, but can still run `Clear Containers`_ directly on top of native `Clear Linux`_ or any other distribution.

Update your Clear Linux
~~~~~~~~~~~~~~~~~~~~~~~

The more recent your version of `Clear Linux`_ the better `Clear Containers`_ will perform, and the general recommendation is that you ensure that you are on the latest version of Clear Linux, or at least version 9890.

To check which version of `Clear Linux`_ you are on, and what the latest available is, from within `Clear Linux`_ run:

  ::

    $ sudo swupd update -s

To update your `Clear Linux`_ installation to the latest execute:

  ::

    $ sudo swupd update

Install Clear Containers bundle
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


  ::

    $ sudo swupd bundle-add containers-basic

Add your user to the KVM and Docker groups
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To enable your user to access both Docker and KVM you will need to add them to the relevant groups on the machine:
 
  ::

    $ sudo usermod -G kvm,docker -a <USERNAME>

Then run the following commands to add those group ids to your active login session:

  ::

    $ newgrp kvm
    $ newgrp docker

Docker on Clear Linux
~~~~~~~~~~~~~~~~~~~~~

Docker_ on `Clear Linux`_  provides 2 service files to start the daemon, and only one will be enabled.

- docker.service (``runc``)
- docker-cor.service (``cc-oci-runtime``)

If you are running `Clear Linux`_ on baremetal or on a VM with `Nested Virtualization`_ activated the service ``docker-cor`` will be launched and default runtime will be ``cc-oci-runtime``.
If you are running `Clear Linux`_ on a VM without `Nested Virtualization`_ the service ``docker`` will be launched and default runtime will be ``runc``.

After you install bundle ``containers-basic`` you'll need to start docker(s) services (don't worry only one of these will start and will do the checks for you) 

  ::

    $ sudo systemctl start docker
    $ sudo systemctl start docker-cor

To check which one of these are activated just run:

  ::

    $ sudo systemctl status docker

    or

    $ sudo systemctl status docker-cor


**Note:** In the next reboot the docker daemon will start automatically.

Final Docker sanity check
~~~~~~~~~~~~~~~~~~~~~~~~~

Before we dive into using `Clear Containers`_ it is prudent to do a final sanity check to ensure that relevant Docker_ parts have installed and are executing correctly:


  ::

    $ docker ps
    $ docker network ls
    $ docker pull busybox 

    $ docker run -it busybox sh
    [    0.063356] systemd[1]: Failed to initialise default hostname
    / # uname -a
     Linux f0098e68456f 4.5.0-49.container #1 SMP Mon Aug 8 20:46:42 UTC 2016 x86_64 GNU/Linux
    / # exit


Conclusion
----------

You now have Docker_ installed with `Clear Containers`_ enabled as the default OCI_ runtime. You can now try out `Clear Containers`_.

.. _Clear Containers: https://clearlinux.org/features/clear-containers

.. _Clear Linux: www.clearlinux.org

.. _Docker: https://www.docker.com/

.. _Installing Clear Linux: https://clearlinux.org/documentation/gs_getting_started.html

.. _OCI: https://www.opencontainers.org/

.. _Nested Virtualization: https://en.wikipedia.org/wiki/Virtualization

