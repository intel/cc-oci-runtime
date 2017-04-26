Name: qemu-lite
Version: 2.7.1
Release: 0
Source0:   %{name}-%{version}.tar.gz
Source1: qemu-lite-rpmlintrc
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}
Summary  : OpenBIOS development utilities
Group    : Development/Tools
License  : BSD-2-Clause BSD-3-Clause GPL-2.0 GPL-2.0+ LGPL-2.0+ LGPL-2.1
Requires: qemu-lite-bin
Requires: qemu-lite-data
BuildRequires : attr-dev
BuildRequires : automake-dev
BuildRequires : bison
BuildRequires : flex
BuildRequires : glib-dev
%if 0%{?centos_version} || 0%{?scientificlinux_version}
BuildRequires : libattr-devel
BuildRequires : libcap-devel
BuildRequires : libcap-ng-devel
%else
BuildRequires : libcap-dev
BuildRequires : libcap-ng-dev
%endif
BuildRequires : libtool
BuildRequires : libtool-dev
BuildRequires : m4
BuildRequires : numactl-dev
BuildRequires : python-dev
BuildRequires : zlib-dev
BuildRequires : pkgconfig(pixman-1)
Patch1: configure.patch

%description
===========
QEMU is a generic and open source machine & userspace emulator and
virtualizer.

%package bin
Summary: bin components for the qemu-lite package.
Group: Binaries
Requires: qemu-lite-data

%description bin
bin components for the qemu-lite package.


%package data
Summary: data components for the qemu-lite package.
Group: Data

%description data
data components for the qemu-lite package.


%prep
#%setup -q -n qemu-2.6.0
%setup -q
%patch1 -p1

%build
export LANG=C
%configure --disable-static --disable-bluez \
--disable-brlapi \
--disable-bzip2 \
--disable-curl \
--disable-curses \
--disable-debug-tcg \
--disable-fdt \
--disable-glusterfs \
--disable-gtk \
--disable-libiscsi \
--disable-libnfs \
--disable-libssh2 \
--disable-libusb \
--disable-linux-aio \
--disable-lzo \
--disable-opengl \
--disable-qom-cast-debug \
--disable-rbd \
--disable-rdma \
--disable-sdl \
--disable-seccomp \
--disable-slirp \
--disable-snappy \
--disable-spice \
--disable-strip \
--disable-tcg-interpreter \
--disable-tcmalloc \
--disable-tools \
--disable-tpm \
--disable-usb-redir \
--disable-uuid \
--disable-vnc \
--disable-vnc-jpeg \
--disable-vnc-png \
--disable-vnc-sasl \
--disable-vte \
--disable-xen \
--enable-attr \
--enable-cap-ng \
--enable-kvm \
--enable-virtfs \
--target-list=x86_64-softmmu \
--extra-cflags="-fno-semantic-interposition -O3 -falign-functions=32" \
--datadir=/usr/share/qemu-lite \
--libdir=/usr/lib64/qemu-lite \
--libexecdir=/usr/libexec/qemu-lite \
--enable-vhost-net \
--disable-docs
make V=1  %{?_smp_mflags}

%install
rm -rf %{buildroot}
%make_install
## make_install_append content
for file in %{buildroot}/usr/bin/*
do
dir=$(dirname "$file")
bin=$(basename "$file")
new=$(echo "$bin"|sed -e 's/qemu-/qemu-lite-/g' -e 's/ivshmem-/ivshmem-lite-/g' -e 's/virtfs-/virtfs-lite-/g')
mv "$file" "$dir/$new"
done
## make_install_append end

%files
%defattr(-,root,root,-)

%files bin
%defattr(-,root,root,-)
/usr/bin/qemu-lite-ga
/usr/bin/qemu-lite-system-x86_64
/usr/bin/virtfs-lite-proxy-helper
%dir /usr/libexec
%dir /usr/libexec/qemu-lite
/usr/libexec/qemu-lite/qemu-bridge-helper

%files data
%defattr(-,root,root,-)
%dir /usr/share/qemu-lite
%dir /usr/share/qemu-lite/qemu
%dir /usr/share/qemu-lite/qemu/keymaps
/usr/share/qemu-lite/qemu/QEMU,cgthree.bin
/usr/share/qemu-lite/qemu/QEMU,tcx.bin
/usr/share/qemu-lite/qemu/acpi-dsdt.aml
/usr/share/qemu-lite/qemu/bamboo.dtb
/usr/share/qemu-lite/qemu/bios-256k.bin
/usr/share/qemu-lite/qemu/bios.bin
/usr/share/qemu-lite/qemu/efi-e1000.rom
/usr/share/qemu-lite/qemu/efi-e1000e.rom
/usr/share/qemu-lite/qemu/efi-eepro100.rom
/usr/share/qemu-lite/qemu/efi-ne2k_pci.rom
/usr/share/qemu-lite/qemu/efi-pcnet.rom
/usr/share/qemu-lite/qemu/efi-rtl8139.rom
/usr/share/qemu-lite/qemu/efi-virtio.rom
/usr/share/qemu-lite/qemu/efi-vmxnet3.rom
/usr/share/qemu-lite/qemu/keymaps/ar
/usr/share/qemu-lite/qemu/keymaps/bepo
/usr/share/qemu-lite/qemu/keymaps/common
/usr/share/qemu-lite/qemu/keymaps/cz
/usr/share/qemu-lite/qemu/keymaps/da
/usr/share/qemu-lite/qemu/keymaps/de
/usr/share/qemu-lite/qemu/keymaps/de-ch
/usr/share/qemu-lite/qemu/keymaps/en-gb
/usr/share/qemu-lite/qemu/keymaps/en-us
/usr/share/qemu-lite/qemu/keymaps/es
/usr/share/qemu-lite/qemu/keymaps/et
/usr/share/qemu-lite/qemu/keymaps/fi
/usr/share/qemu-lite/qemu/keymaps/fo
/usr/share/qemu-lite/qemu/keymaps/fr
/usr/share/qemu-lite/qemu/keymaps/fr-be
/usr/share/qemu-lite/qemu/keymaps/fr-ca
/usr/share/qemu-lite/qemu/keymaps/fr-ch
/usr/share/qemu-lite/qemu/keymaps/hr
/usr/share/qemu-lite/qemu/keymaps/hu
/usr/share/qemu-lite/qemu/keymaps/is
/usr/share/qemu-lite/qemu/keymaps/it
/usr/share/qemu-lite/qemu/keymaps/ja
/usr/share/qemu-lite/qemu/keymaps/lt
/usr/share/qemu-lite/qemu/keymaps/lv
/usr/share/qemu-lite/qemu/keymaps/mk
/usr/share/qemu-lite/qemu/keymaps/modifiers
/usr/share/qemu-lite/qemu/keymaps/nl
/usr/share/qemu-lite/qemu/keymaps/nl-be
/usr/share/qemu-lite/qemu/keymaps/no
/usr/share/qemu-lite/qemu/keymaps/pl
/usr/share/qemu-lite/qemu/keymaps/pt
/usr/share/qemu-lite/qemu/keymaps/pt-br
/usr/share/qemu-lite/qemu/keymaps/ru
/usr/share/qemu-lite/qemu/keymaps/sl
/usr/share/qemu-lite/qemu/keymaps/sv
/usr/share/qemu-lite/qemu/keymaps/th
/usr/share/qemu-lite/qemu/keymaps/tr
/usr/share/qemu-lite/qemu/kvmvapic.bin
/usr/share/qemu-lite/qemu/linuxboot.bin
/usr/share/qemu-lite/qemu/linuxboot_dma.bin
/usr/share/qemu-lite/qemu/multiboot.bin
/usr/share/qemu-lite/qemu/openbios-ppc
/usr/share/qemu-lite/qemu/openbios-sparc32
/usr/share/qemu-lite/qemu/openbios-sparc64
/usr/share/qemu-lite/qemu/palcode-clipper
/usr/share/qemu-lite/qemu/petalogix-ml605.dtb
/usr/share/qemu-lite/qemu/petalogix-s3adsp1800.dtb
/usr/share/qemu-lite/qemu/ppc_rom.bin
/usr/share/qemu-lite/qemu/pxe-e1000.rom
/usr/share/qemu-lite/qemu/pxe-eepro100.rom
/usr/share/qemu-lite/qemu/pxe-ne2k_pci.rom
/usr/share/qemu-lite/qemu/pxe-pcnet.rom
/usr/share/qemu-lite/qemu/pxe-rtl8139.rom
/usr/share/qemu-lite/qemu/pxe-virtio.rom
/usr/share/qemu-lite/qemu/qemu-icon.bmp
/usr/share/qemu-lite/qemu/qemu_logo_no_text.svg
/usr/share/qemu-lite/qemu/s390-ccw.img
/usr/share/qemu-lite/qemu/sgabios.bin
/usr/share/qemu-lite/qemu/slof.bin
/usr/share/qemu-lite/qemu/spapr-rtas.bin
/usr/share/qemu-lite/qemu/trace-events-all
/usr/share/qemu-lite/qemu/u-boot.e500
/usr/share/qemu-lite/qemu/vgabios-cirrus.bin
/usr/share/qemu-lite/qemu/vgabios-qxl.bin
/usr/share/qemu-lite/qemu/vgabios-stdvga.bin
/usr/share/qemu-lite/qemu/vgabios-virtio.bin
/usr/share/qemu-lite/qemu/vgabios-vmware.bin
/usr/share/qemu-lite/qemu/vgabios.bin
