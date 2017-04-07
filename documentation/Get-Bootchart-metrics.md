# Get Bootchart from a Clear Container #

The following steps describe how to get a bootchart from a Clear Containers
container. Currently `systemd` is the default init process in the Clear Containers mini-os
system. The systemd project provides a tool called `systemd-bootchart` that
collects the CPU load, disk load, memory usage, as well as per-process information
about the os boot process.

## Configure Clear Containers to get a bootchart ##

To get a bootchart from a Clear Container, follow the steps below.

1. Prepare your Clear Container image:

Use the file data/cc-bootchart.conf to get a bootchart graph.

To install the configuration file in a Clear Container image with systemd and
systemd-bootchart, use the configure option `--with-cc-image-bootchart-config-dir`.

```
#Change ROOTFS value to rootfs container image
ROOTFS="dir/to/cc_rootfs"
BOOTCHART_CONFIG_DIR="${ROOTFS}/usr/lib/systemd/bootchart.conf.d/"

./autogen.sh --with-cc-image-bootchart-config-dir="${BOOTCHART_CONFIG_DIR}"
```

2. Configure Clear Containers to boot with bootchart:

Make sure your kernel uses `systemd-bootchart` as the init process (PID 1).

* Option 1: Using a packaged version of `cc-oci-runtime`

    1. Copy the file `/usr/share/defaults/cc-oci-runtime/vm.json` to
       `/etc/cc-oci-runtime/vm.json` (only if it does not exist).

    2. Replace in `/etc/cc-oci-runtime/vm.json` `init=/usr/lib/systemd/systemd` to
       `init=/usr/lib/systemd/systemd-bootchart`.

  Example:

  ```
  sudo mkdir -p /etc/cc-oci-runtime/
  [ -f /etc/cc-oci-runtime/vm.json ] || \
      sudo cp /usr/share/defaults/cc-oci-runtime/vm.json /etc/cc-oci-runtime/vm.json
  sudo sed -i 's,init=/usr/lib/systemd/systemd,init=/usr/lib/systemd/systemd-bootchart,g'\
      /etc/cc-oci-runtime/vm.json
  ```

* Option 2: Installing `cc-oci-runtime` from source code

  1. Replace `init=/usr/lib/systemd/systemd` to `init=PATH TO SYSTEMD-BOOTCHART BINARY`
     in the file `data/kernel-cmdline`. 

  2. Install `cc-oci-runtime` using `make install`.

     Example:

     ```
     sed -i 's,init=/usr/lib/systemd/systemd,init=/usr/lib/systemd/systemd-bootchart,g' \
     data/kernel-cmdline
     ./autogen.sh
     make
     make install
     ```

3. Extract bootchart from a container:

The Clear Containers system is now configured to use systemd-bootchart and
all newly created containers will create a bootchart on boot at this location:

`/run/bootchart-*.svg`

Example:

```
# wait some time if svg does not exist.
docker run -ti debian bash -c \
        't=0 ;
        set -x;
        while [ ! -f /run/*.svg ] && [ "$t" -lt 10 ];
        do
                sleep 1;
                echo  "wait for bootchart" ;
                t=$((t+1));
        done;
        cat /run/*.svg' > bootchart.svg
```
