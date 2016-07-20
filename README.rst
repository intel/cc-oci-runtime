.. contents::
.. sectnum::

``cc-oci-runtime``
===================

Overview
--------

``cc-oci-runtime`` is an Open Containers Initiative (OCI_) "runtime"
that launches an Intel_ VT-x secured Clear Containers 2.0 hypervisor,
rather than a standard Linux container. It leverages the highly
optimised `Clear Linux`_ technology to achieve this goal.

The tool is licensed under the terms of the GNU GPLv2 and aims to be
compatible with the OCI_ runtime specification [#oci-spec]_, allowing
Clear Containers to be launched transparently by Docker_ (using
containerd_) and other OCI_-conforming container managers.

Henceforth, the tool will simply be referred to as "the runtime".

See the canonical `cc-oci-runtime home page`_ for the latest
information.

Requirements
------------

- A Qemu_ hypervisor that supports the ``pc-lite`` machine type (see qemu-lite_).

Platform Support
----------------

the runtime supports running Clear Containers on Intel 64-bit (x86-64)
Linux systems.

Supported Application Versions
------------------------------

The runtime has been tested with the following application versions:

- Docker_ version 1.12-rc4.
- Containerd_ version 0.2.2.

Limitations
-----------

Since this is early code and is tracking both Docker_ and an unratified
specification, there are still a few feature gaps.

The remainder of this section documents those gaps, all of which
are being worked on.

Non-interactive "``docker run``" and Docker_ logs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If the runtime is configured for docker and "``docker run``" is
specified without "``-ti``", no output is currently generated to either
the console or the docker log ("``docker logs``" command).

See https://github.com/01org/cc-oci-runtime/issues/17

Networking
~~~~~~~~~~

Networking within the Clear Container is not currently available.

Networking support is complicated by the way Dockers networking code
assumes the runtime will create a traditional container.

This feature is currently WIP and is expected to land soon - a branch
containing experimental code can be found here:

- https://github.com/01org/cc-oci-runtime/tree/networking

See https://github.com/01org/cc-oci-runtime/issues/38

``exec``
~~~~~~~~

The ``exec`` command that allows a new process to run inside a container
is not fully implemented, in part due to the fact that `Networking`_ is not
yet available.

See https://github.com/01org/cc-oci-runtime/issues/18

Hostname
~~~~~~~~

The hostname specified in the OCI_ configuration file is not currently
applied to the Clear Container, in part due to the fact that
`Networking`_ is not yet available.

See https://github.com/01org/cc-oci-runtime/issues/19

Running a workload as a non-``root`` user/group
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The runtime currently ignores the request to run the workload as a
non-root user and/or group, defaulting to running as the "``root``" user
(inside the Clear Container).

See https://github.com/01org/cc-oci-runtime/issues/20

Annotations
~~~~~~~~~~~

OCI_ Annotations are not currently exposed inside the Clear Container.

See https://github.com/01org/cc-oci-runtime/issues/21

No checkpoint and restore commands
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Although the runtime provides stub implementations of these commands,
this is currently purely to satisfy Docker_ - the commands do *NOT*
save/restore the state of the Clear Container.

See https://github.com/01org/cc-oci-runtime/issues/22

Running under ``docker``
------------------------

Assuming a Docker_ 1.12 environment, start the Docker_ daemon specifying
the "``--add-runtime $alias=$path``" option. For example::

    $ sudo dockerd --add-runtime cor=/usr/bin/cc-oci-runtime

Then, to run a Clear Container using the runtime, specify "``--runtime cor``".

For example::

    $ sudo docker-run --runtime cor -ti busybox

Running under ``containerd`` (without Docker)
---------------------------------------------

If you are running Containerd_ directly, without Docker_:

- Start the server daemon::

    $ sudo /usr/local/bin/containerd --debug --runtime $PWD/cc-oci-runtime

- Launch a hypervisor::

    $ name=foo

    # XXX: path to directory containing atleast the following:
    #
    #   config.json
    #   rootfs/
    #
    $ bundle_dir=...

    $ sudo /usr/local/bin/ctr --debug containers start --attach "$name" "$bundle_dir"

- Forcibly stop the hypervisor::

    $ name=foo
    $ sudo ./cc-oci-runtime stop "$name"

Running stand-alone
-------------------

The runtime can be run directly, without the need for either ``docker``
or ``containerd``::

    $ name=foo
    $ pidfile=/tmp/cor.pid
    $ logfile=/tmp/cor.log
    $ sudo ./cc-oci-runtime --debug create --console $(tty) --bundle "$bundle_dir" "$name"
    $ sudo ./cc-oci-runtime --debug start "$name"

Or, to simulate how ``containerd`` calls the runtime::

    $ sudo ./cc-oci-runtime --log "$logfile" --log-format json create --bundle "$bundle_dir" --console $(tty) -d --pid-file "$pidfile" "$name"
    $ sudo ./cc-oci-runtime --log "$logfile" --log-format json start "$name"

Running as a non-privileged user
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Assuming the following provisos, the runtime can be run as a non-``root`` user:

- User has read+write permissions for the Clear Containers root
  filesystem image specified in the ``vm`` JSON object (see
  Configuration_).

- User has read+execute permissions for the Clear Containers kernel
  image specified in the ``vm`` JSON object (see Configuration_).

- The bundle configuration file ("``config.json``") does not specify any
  mounts that the runtime must honour.

- The runtime is invoked with the "``--root=$dir``" option where
  "``$dir``" is a pre-existing directory that the user has write
  permission to.

To run non-privileged::

    $ name=foo
    $ dir=/tmp/cor
    $ mkdir -p "$dir"
    $ ./cc-oci-runtime --root "$dir" create --console $(tty) --bundle "$oci_bundle_directory" "$name"
    $ ./cc-oci-runtime --root "$dir" start "$name"

Community
---------

See `the contributing page <https://github.com/01org/cc-oci-runtime/blob/master/CONTRIBUTING.md#contact>`_.

Building
--------

Dependencies
~~~~~~~~~~~~

Ensure you have the development versions of the following packages
installed on your system:

- check
- glib
- json-glib
- uuid

Configure Stage
~~~~~~~~~~~~~~~

Quick start, just run::

  $ ./autogen.sh && make

If you have specific requirements, run::

  $ ./configure --help

... then add the extra "``configure``" flags you want to use::

  $ ./autogen.sh --enable-foo --disable-bar && make

Tests
-----

To run the basic unit tests, run::

  $ make check

To configure the command above to also run the functional tests
(recommended), see the `functional tests README`_.

Configuration
-------------

At the time of writing, the OCI_ had not agreed on how best to handle
VM-based runtimes such as this (see [#oci-vm-config-issue]_).

Until the OCI_ specification clarifies how VM runtimes will be defined,
the runtime will search a number of different data sources for its VM
configuration information.

Unless otherwise specified, each configuration file in this section will
be looked for in the following directories (in order):

- The bundle directory, specified by "``--bundle $bundle_dir``".

- The system configuration directory ("``./configure --sysconfdir=...``").
  
  With no ``--prefix`` or with ``--prefix=/``, the file will be looked
  for in ``/etc/cc-oci-runtime/``".

- The defaults directory.
 
  This is a directory called "``defaults/cc-oci-runtime/``" below the
  configured data directory ("``./configure --datadir=...``").
  
  With no ``--prefix`` or with ``--prefix=/``, the file will be looked
  for in ``/usr/share/defaults/cc-oci-runtime/``".

The first file found will be used and the runtime will log the full path
to each configuration file used (see `Logging`_).

Example files will be available in the "``data/``" directory after the
build has completed. To influence the way these files are generated,
consider using the following "``configure``" options:

- ``--with-qemu-path=``
- ``--with-cc-kernel=``
- ``--with-cc-image=``

.. note:: You may still need to make adjustments to this file to work
   for your environment.

``config.json``
~~~~~~~~~~~~~~~

The runtime will consult the OCI configuration file ``config.json``
for a "``vm``" object, according to the proposed OCI specification
[#oci-vm-config-issue]_

``vm.json``
~~~~~~~~~~~

If no "``vm``" object is found in ``config.json``, the file
"``vm.json``" will be looked for which should contain a stand-alone
JSON "``vm``" object specifying the virtual machine configuration.

``hypervisor.args``
~~~~~~~~~~~~~~~~~~~

This file specifies both the full path to the hypervisor binary to use
and all the arguments to be passed to it. The runtime supports
specifying specific options using variables (see `Variable Expansion`_).

Variable Expansion
..................

Currently, the runtime will expand the following `special tags` found in
``hypervisor.args`` appropriately:

- ``@COMMS_SOCKET@`` - path to the hypervisor control socket (QMP socket for qemu).
- ``@CONSOLE_DEVICE@`` - hypervisor arguments used to control where console I/O is sent to.
- ``@IMAGE@`` - Clear Containers rootfs image path (read from ``config.json``).
- ``@KERNEL_PARAMS@`` - kernel parameters (from ``config.json``).
- ``@KERNEL@`` - path to kernel (from ``config.json``).
- ``@NAME@`` - VM name.
- ``@PROCESS_SOCKET@`` - required to detect efficiently when hypervisor is shut down.
- ``@SIZE@`` - size of @IMAGE@ which is auto-calculated.
- ``@UUID@`` - VM uuid.
- ``@WORKLOAD_DIR@`` - path to workload chroot directory that will be mounted (via 9p) inside the VM.

Logging
-------

The runtime logs to the file specified by the global ``--log`` option.
However, it can also write to a global log file if the
``--global-log`` option is specified. Note that if both log options are
specified, both log files will be appended to.

The global log potentially provides more detail than the standard log
since it is always written to in ASCII format and includes Process ID
details. Also note that all instances of the runtime will append to
the global log.

The global log file is named ``cc-oci-runtime.log``, and will be
written into the directory specified by "``--root``".  The default
runtime state directory is ``/run/opencontainer/containers/`` if no
"``--root``" argument is supplied.

Note: Global logging is presently always enabled in the runtime,
as ``containerd`` does not always invoke the runtime with the ``--log``
argument, and enabling the global log in this case helps with debugging.

Command-line Interface
----------------------

At the time of writing, the OCI_ has provided recommendations for the
runtime command line interface (CLI) (see [#oci-runtime-cli]_).

However, the OCI_ runtime reference implementation, runc_, has a CLI
which deviates from the recommendations.

This issue has been raised with OCI_ (see [#oci-runtime-cli-clarification]_), but
until the situation is clarified, the runtime strives to support both
the OCI_ CLI and the runc_ CLI interfaces.

Details of the runc_ command line options can be found in the `runc manpage`_.

Note: The ``--global-log`` argument is unique to the runtime at present.

Extensions
~~~~~~~~~~

list
....

The ``list`` command supports a "``--all``" option that provides
additional information including details of the resources used by the
virtual machine.

Development
-----------

Follow the instructions in `Building`_, but you will also want to install:

- doxygen
- lcov
- valgrind

To build the API documentation::

  $ doxygen Doxyfile

Then, point your browser at ``/tmp/doxygen-cc-oci-runtime``. If you
don't like that location, change the value of ``OUTPUT_DIRECTORY`` in
the file ``Doxyfile``.

Debugging
---------

- Specify the ``--enable-debug`` configure option to the ``autogen.sh``
  script which enable debug output, but also disable all compiler and
  linker optimisations.

- If you want to see the hypervisor boot messages, remove "`quiet`" from
  the hypervisor command-line in "``hypervisor.args``".

- Run with the "``--debug``" global option.

- If you want to debug as a non-root user, specify the "``--root``"
  global option. For example::

    $ gdb --args ./cc-oci-runtime \
        --debug \
        --root /tmp/cor/ \
        --global-log /tmp/global.log \
        start --console $(tty) $container $bundle_path

- Consult the global Log (see Logging_).

Links
-----

.. _Intel: https://www.intel.com

.. _`Clear Linux`: https://clearlinux.org/

.. _`Qemu`: http://qemu.org

.. _`qemu-lite`: https://github.com/01org/qemu-lite

.. _OCI: https://www.opencontainers.org/

.. _`cc-oci-runtime home page`: https://github.com/01org/cc-oci-runtime

.. _runc: https://github.com/opencontainers/runc

.. _`runc manpage`: https://github.com/opencontainers/runc/blob/master/man/runc.8.md`

.. _Docker: https://github.com/docker/docker

.. _containerd: https://github.com/docker/containerd

.. [#oci-spec]
   https://github.com/opencontainers/runtime-spec

.. [#oci-runtime-cli]
   https://github.com/opencontainers/runtime-spec/blob/master/runtime.md

.. [#oci-vm-config-issue]
   https://github.com/opencontainers/runtime-spec/pull/405

.. [#oci-runtime-cli-clarification]
   https://github.com/opencontainers/runtime-spec/issues/434

.. _`functional tests README`: https://github.com/01org/cc-oci-runtime/tree/master/tests/functional/README.rst
