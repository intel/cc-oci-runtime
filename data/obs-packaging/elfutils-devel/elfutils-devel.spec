Name     : elfutils-devel
Version  : 0.166
Release  : 30
URL      : https://fedorahosted.org/releases/e/l/elfutils/0.166/elfutils-0.166.tar.bz2
Summary  : A collection of utilities and DSOs to handle compiled objects
Group    : Development/Tools
License  : GPL-2.0 GPL-2.0+ GPL-3.0 GPL-3.0+ LGPL-3.0 LGPL-3.0+
Requires: libasm-devel
Requires: libebl-devel
Requires: libelf-devel
Requires: libdw-devel
Provides: openssl-dev

%description
This package provides a higher-level library to access ELF files. This
is a part of elfutils package.

%if 0%{?suse_version}
%package -n btrfs-progs-devel
Requires: libbtrfs-devel
Summary  : A library for btrfs
Group    : Development/Tools

%description -n btrfs-progs-devel
This package provides a higher-level library btrfs.
%endif

%package -n openssl-dev
%if 0%{?suse_version}
Requires : libopenssl-devel
%else
Requires : openssl-devel
%endif
Summary  : openssl development package
Group    : Development/Tools

%description -n openssl-dev
This package provides libraries and include files needed to compile apps with support
for various cryptographic algorithms and protocols.

%package -n glibc-staticdev
%if 0%{?suse_version}
Requires: glibc-devel-static
%else
Requires: glibc-static
%endif
Summary  : A static version of glibc
Group    : Development/Tools

%description -n glibc-staticdev
This package provides a static libc library

%prep
%build
%install
%if 0%{?suse_version}
mkdir -p %{buildroot}/usr/share/doc/elfutils-devel
touch %{buildroot}/usr/share/doc/elfutils-devel/dummy
mkdir -p %{buildroot}/usr/share/doc/btrfs-progs-devel
touch %{buildroot}/usr/share/doc/btrfs-progs-devel/dummy
%endif
mkdir -p %{buildroot}/usr/share/doc/glibc-staticdev
touch %{buildroot}/usr/share/doc/glibc-staticdev/dummy
mkdir -p %{buildroot}/usr/share/doc/openssl-dev
touch %{buildroot}/usr/share/doc/openssl-dev/dummy

%if 0%{?suse_version}
%files
%dir /usr/share/doc/elfutils-devel
/usr/share/doc/elfutils-devel/dummy
%files -n btrfs-progs-devel
%dir /usr/share/doc/btrfs-progs-devel
/usr/share/doc/btrfs-progs-devel/dummy
%endif
%files -n glibc-staticdev
%dir /usr/share/doc/glibc-staticdev
/usr/share/doc/glibc-staticdev/dummy
%files -n openssl-dev
%dir /usr/share/doc/openssl-dev
/usr/share/doc/openssl-dev/dummy
