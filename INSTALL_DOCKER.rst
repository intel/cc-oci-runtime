.. contents::
.. sectnum::

``Installing Clear Containers 2.0 into Clear Linux``
====================================================

Introduction
------------
.. attention::
   This document is a work in progress - the **Attention!** boxes note places where things may be updated as infrastructure develops.

`Clear Containers`_ 2.0 provides an Open Containers Initiative (OCI_) compatible 'runtime' and is installable into Docker_ 1.12.0 and later, where OCI_ runtime support is available.

This document details how to install Docker-1.12.0 and the necessary parts of `Clear Containers`_  into a `Clear Linux`_ distribution.

You will need to have a `Clear Linux`_ installation before commencing this procedure, although `Clear Containers`_ do not depend on `Clear Linux`_ as a host and can be run on top of other distributions. See `Installing Clear Linux`_ for more details.


Overview
--------
The following steps install and configure `Clear Containers`_ and Docker_ into an existing `Clear Linux`_ distribution. You will require `Clear Linux`_ version 8620 or above.

Again, please note that `Clear Containers`_ can run on top of other distributions. Here, `Clear Linux`_ is used as one example of a distribution `Clear Containers`_ can run on. This document does not cover installing `Clear Containers`_ on other distributions.

After this installation you will be able to launch Docker_ container payloads using either the default Docker_ (``runc``) Linux Container runtime or the `Clear Containers`_ QEMU/KVM hypervisor based runtime - ``cc-oci-runtime``.

Installation Steps
------------------

Enable sudo
~~~~~~~~~~~

You will need root privileges in order to run a number of the following commands. It is recommended you run these commands from a user account with ``sudo`` rights. 

If your user does not already have ``sudo`` rights, you should add your user to ``wheel`` group whilst logged in as ``root``:

  ::

    usermod -G wheel -a <USERNAME>

This change will take effect once you have logged out as root and logged back in as <USERNAME>.

And you will also need to add your user or group to the ``/etc/sudoers`` file, for example:

  ::

    visudo
    #and add the line:
      %wheel  ALL=(ALL)    ALL

You can now log out of ``root``, log in as your <USERNAME> and continue by using ``sudo``.

Check Clear Container compatability
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Before you try to install and run `Clear Containers`_ it is prudent to check that your machine (hardware) is compatible. The easiest way to do this is to download and run the Clear Containers check config script:

  ::

    curl -O https://download.clearlinux.org/current/clear-linux-check-config.sh
    chmod +x clear-linux-check-config.sh
    ./clear-linux-check-config.sh container

This command will print a list of test results. All items should return a 'SUCCESS' status - but you can ignore the 'Nested KVM support' item if it fails - this just means you cannot run `Clear Containers`_ under another hypervisor such as KVM, but can still run `Clear Containers`_ directly on top of native `Clear Linux`_ or any other distribution.

Update your Clear Linux
~~~~~~~~~~~~~~~~~~~~~~~

The more recent your version of `Clear Linux`_ the better `Clear Containers`_ will perform, and the general recommendation is that you ensure that you are on the latest version of Clear Linux, or at least version 8620.

To check which version of `Clear Linux`_ you are on, and what the latest available is, from within `Clear Linux`_ run:

  ::

    swupd update -s

To update your `Clear Linux`_ installation to the latest execute:

  ::

    sudo swupd update

Uninstall container-basic
~~~~~~~~~~~~~~~~~~~~~~~~~

If you are on an older version of `Clear Linux`_ you may have an old version of the ``container-basic`` bundle installed, that contains an older version of Docker_. If so, remove this bundle:

  ::

    sudo swupd bundle-remove containers-basic

Install Clear Containers bundles
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. attention::
   Once appropriate bundles become availabel on Clear Linux the below installation commands will be updated.

Install the following bundles and RPMs to enable our work in progress linux-container-testing packages.

  ::

    sudo swupd bundle-add os-clr-on-clr
    sudo swupd bundle-add os-core-dev
    sudo swupd bundle-add os-dev-extras
    sudo rpm -ivh --nodeps --force https://download.clearlinux.org/current/x86_64/os/Packages/qemu-lite-bin-2.6.0-17.x86_64.rpm
    sudo rpm -ivh --nodeps --force https://download.clearlinux.org/current/x86_64/os/Packages/qemu-lite-data-2.6.0-17.x86_64.rpm
    sudo rpm -ivh --nodeps --force https://download.clearlinux.org/current/x86_64/os/Packages/json-glib-dev-1.2.0-8.x86_64.rpm
    sudo rpm -ivh --nodeps --force https://download.clearlinux.org/current/x86_64/os/Packages/json-glib-lib-1.2.0-8.x86_64.rpm
    sudo rpm -ivh --nodeps --force https://download.clearlinux.org/current/x86_64/os/Packages/linux-container-testing-4.5-9.x86_64.rpm
    sudo rpm -ivh --nodeps --force https://download.clearlinux.org/current/x86_64/os/Packages/linux-container-testing-extra-4.5-9.x86_64.rpm
    sudo rpm -ivh --nodeps --force https://download.clearlinux.org/current/x86_64/os/Packages/iproute2-dev-4.3.0-25.x86_64.rpm
    #Note: Ignore the errorldconfig:*
    #/usr/lib64/libguile-2.0.so.22.7.2-gdb.scm is not an ELF file - it has the wrong magic bytes at the start.*

Check QEMU-lite
~~~~~~~~~~~~~~~

`Clear Containers`_ uses an optimised version of `QEMU`_ called `QEMU-lite`_
You can now check that the `QEMU-lite`_ package is installed and functioning:

  ::

    # qemu-lite-system-x86_64 --version
    QEMU emulator version 2.6.0, Copyright (c) 2003-2008 Fabrice Bellard

    # qemu-lite-system-x86_64 --machine help | grep pc-lite
    pc-lite Light weight PC (alias of pc-lite-2.6)

    pc-lite-2.6Light weight PC

Install the Docker bundle
~~~~~~~~~~~~~~~~~~~~~~~~~

We can now install the `Clear Linux`_ bundle that containers Docker_:

  ::

    sudo swupd bundle-add opencontainers-dev

Add your user to the KVM and Docker groups
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To enable your user to access both Docker and KVM you will need to add them to the relevant groups on the machine:
 
  ::

    sudo usermod -G kvm,docker -a <USERNAME>

Then run the following commands to add those group ids to your active login session:

  ::

    newgrp kvm
    newgrp docker

Download the cc-oci-runtime 2.0 code
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. attention::
   Update this section when cc-oci-runtime is available in a Clear Linux bundle

Download, build and install the ``cc-oci-runtime`` from source:

  ::

    cor_source=${HOME}/cc-oci-runtime
    git clone https://github.com/01org/cc-oci-runtime.git $cor_source
    cd $cor_source
    bash autogen.sh --disable-cppcheck --disable-valgrind
    make
    sudo make install

Restart Docker
~~~~~~~~~~~~~~

In order to ensure you are running the latest installed Docker_ you should restart the Docker_ daemon:

  ::

    sudo systemctl daemon-reload
    sudo systemctl restart docker-upstream

Final Docker sanity check
~~~~~~~~~~~~~~~~~~~~~~~~~

Before we dive into using `Clear Containers`_ it is prudent to do a final sanity check to ensure that relevant Docker_ parts have installed and are executing correctly:

  ::

    sudo systemctl status docker-upstream
    docker-upstream ps
    docker-upstream network ls
    docker-upstream pull debian
    docker-upstream run -it debian

.. attention::
   We should put the example output text in this section to aid clarity.

If these tests pass then you have a working Docker_, and thus a good baseline to evaluate `Clear Containers`_ under.

Enable Clear Containers runtime
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Now we have `Clear Containers`_ and Docker_ installed we need to tie them together by enabling the `Clear Containers`_ runtime within the Docker_ system:

Locate where your OCI runtime got installed

    ::

      which cc-oci-runtime
      #typically /usr/bin/cc-oci-runtime

Then create a custom Docker_ systemd unit file to make `Clear Containers`_ the default runtime.

  ::

    sudo mkdir -p /etc/systemd/system/docker-upstream.service.d
    cat << EOF | sudo tee -a /etc/systemd/system/docker-upstream.service.d/driver.conf
    [Service]
    ExecStart=
    ExecStart=/usr/bin/dockerd-upstream --add-runtime cor=/usr/bin/cc-oci-runtime  --default-runtime=cor -H fd:// --storage-driver=overlay
    EOF


Install the Clear Container container kernel image
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`Clear Containers`_ utilise a root filesystem and Linux kernel image to run the Docker_ container payloads. The kernel was installed by the linux-container-testing RPM. The root filesystem image can be obtained from the `Clear Linux`_ download site. The runtime default config file for they hypervisor can be copied into place from the ``cc-oci-runtime`` source tree:

  ::

    sudo mkdir -p /var/lib/cc-oci-runtime/data/{image,kernel}
    cd /var/lib/cc-oci-runtime/data/image/
    latest=`curl -s https://download.clearlinux.org/latest`
    sudo curl -O https://download.clearlinux.org/releases/${latest}/clear/clear-${latest}-containers.img.xz
    sudo unxz clear-${latest}-containers.img.xz
    sudo cp -s clear-${latest}-containers.img clear-containers.img
    sudo cp -s /usr/lib/kernel/vmlinux-4.5-9.container.testing /var/lib/cc-oci-runtime/data/kernel/vmlinux.container

Restart Docker Again
~~~~~~~~~~~~~~~~~~~~

In order for the changes to take effect (and verify that the new parameters are in effect) we need to restart the Docker_ daemon again:

  ::

    sudo systemctl daemon-reload
    sudo systemctl restart docker-upstream
    sudo systemctl status docker-upstream

Verify the runtime
~~~~~~~~~~~~~~~~~~

You can now verify that you can launch Docker_ containers with the `Clear Containers`_ runtime:
 
  ::

    sudo docker-upstream run -it debian

.. attention::
   Detail the expected result here for clarity. It may be simpler to start a 'uname -a' or similar and that will visibly show we are running a Clear Container kernel

Conclusion
----------

You now have Docker_ installed with `Clear Containers`_ enabled as the default OCI_ runtime. You can now try out `Clear Containers`_.

.. _Clear Containers: https://clearlinux.org/features/clear-containers

.. _Clear Linux: www.clearlinux.org

.. _Docker: https://www.docker.com/

.. _Installing Clear Linux: https://clearlinux.org/documentation/gs_getting_started.html

.. _OCI: https://www.opencontainers.org/

.. _QEMU: http://wiki.qemu.org/Main_Page

.. _QEMU-lite: http://github.com/01org/qemu-lite

