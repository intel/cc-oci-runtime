Name     : attr-dev
Version  : 2.4.47
Release  : 28
URL      : http://download.savannah.gnu.org/releases/attr/attr-2.4.47.src.tar.gz 
Summary  : No detailed summary available
Group    : Development/Tools
License  : GPL-2.0 GPL-2.0+ LGPL-2.1 
Requires: libattr-devel

%description
This package contains the libraries and header files needed to develop
programs which make use of extended attributes.

%package -n automake-dev
Requires : automake
Summary  : Automatically generate GNU standard Makefiles
Group    : Development/Tools

%description -n automake-dev
Automake is a tool for automatically generating `Makefile.in'
files compliant with the GNU Coding Standards.

%package -n glib-dev
Requires : glib2-devel
Summary  : General purpose utlity library
Group    : Development/Tools

%description -n glib-dev
GLib is a general-purpose utility library, which provides many useful
data types, macros, type conversions, string utilities, file utilities,
a main loop abstraction, and so on.

%package -n libcap-dev
Requires : libcap-devel
Summary  : Development files (Headers, libraries for static linking, etc) for libcap.
Group    : Development/Tools

%description -n libcap-dev
libcap is a library for getting and setting POSIX.1e (formerly POSIX 6)
draft 15 capabilities.

%package -n libcap-ng-dev
Requires : libcap-ng-devel
Summary  : Development files (Headers, libraries for static linking, etc) for libcap-ng.
Group    : Development/Tools

%description -n libcap-ng-dev
The libcap-ng-devel package contains the files needed for developing
applications that need to use the libcap-ng library.

%package -n libtool-dev
%if 0%{?suse_version}
Requires : libtool
%else
Requires : libtool-ltdl-devel
Requires : libtool
%endif
Summary  : Development files for libtool.
Group    : Development/Tools

%description -n libtool-dev
GNU Libtool is a set of shell scripts which automatically configure UNIX and
UNIX-like systems to generically build shared libraries.

%package -n numactl-dev
%if 0%{?suse_version}
Requires : libnuma1
Requires : numactl
%else
Requires : numactl-devel
%endif
Summary  : development headers for numa library calls
Group    : Development/Tools

%description -n numactl-dev
development headers for numa library calls

%package -n python-dev
Requires : python-devel
Summary  : Python Interpreter shared library
Group    : Development/Tools

%description -n python-dev
Python Interpreter shared library

%package -n zlib-dev
Requires : zlib
Summary  : The compression and decompression library
Group    : Development/Tools

%description -n zlib-dev
The compression and decompression library

%package -n libpng-dev
#%if 0%{?centos_version}
#Prefer: -libpng12-devel
#%endif
%if 0%{?suse_version}
Requires : libpng16-devel
%else
Requires : libpng-devel
%endif
Summary  : Development Tools for applications which will use the Libpng library
Group    : Development/Tools

%description -n libpng-dev
Development Tools for applications which will use the Libpng library

%package -n gtk-doc-dev
Requires : gtk-doc
Summary  : Tool for generating API reference documentation 
Group    : Development/Tools

%description -n gtk-doc-dev
Tool for generating API reference documentation

%package -n libxml2-dev
Requires : libxml2-devel
Summary  : Files and Libraries mandatory for Development
Group    : Development/Tools

%description -n libxml2-dev
Files and Libraries mandatory for Development

%package -n libxslt-bin
Requires : libxslt
Summary  : Library providing the Gnome XSLT engine
Group    : Development/Tools

%description -n libxslt-bin
Library providing the Gnome XSLT engine

%package -n docbook-xml
%if 0%{?suse_version}
Requires : docbook-xsl-stylesheets
%else
Requires : docbook-style-xsl
%endif
Summary  : No detailed summary available
Group    : Development/Tools

%description -n docbook-xml
No detailed description available

%package -n elfutils-dev
Requires : elfutils-devel
Summary  : Access to elf files
Group    : Development/Tools

%description -n elfutils-dev
This package provides a higher-level library to access ELF files. This
is a part of elfutils package.


%prep
%build
%install
mkdir -p %{buildroot}/usr/share/doc/attr-dev
touch %{buildroot}/usr/share/doc/attr-dev/dummy
mkdir -p %{buildroot}/usr/share/doc/automake-dev
touch %{buildroot}/usr/share/doc/automake-dev/dummy
mkdir -p %{buildroot}/usr/share/doc/glib-dev
touch %{buildroot}/usr/share/doc/glib-dev/dummy
mkdir -p %{buildroot}/usr/share/doc/libcap-dev
touch %{buildroot}/usr/share/doc/libcap-dev/dummy
mkdir -p %{buildroot}/usr/share/doc/libcap-ng-dev
touch %{buildroot}/usr/share/doc/libcap-ng-dev/dummy
mkdir -p %{buildroot}/usr/share/doc/libtool-dev
touch %{buildroot}/usr/share/doc/libtool-dev/dummy
mkdir -p %{buildroot}/usr/share/doc/numactl-dev
touch %{buildroot}/usr/share/doc/numactl-dev/dummy
mkdir -p %{buildroot}/usr/share/doc/python-dev
touch %{buildroot}/usr/share/doc/python-dev/dummy
mkdir -p %{buildroot}/usr/share/doc/zlib-dev
touch %{buildroot}/usr/share/doc/zlib-dev/dummy
mkdir -p %{buildroot}/usr/share/doc/libpng-dev
touch %{buildroot}/usr/share/doc/libpng-dev/dummy
mkdir -p %{buildroot}/usr/share/doc/gtk-doc-dev
touch %{buildroot}/usr/share/doc/gtk-doc-dev/dummy
mkdir -p %{buildroot}/usr/share/doc/libxml2-dev
touch %{buildroot}/usr/share/doc/libxml2-dev/dummy
mkdir -p %{buildroot}/usr/share/doc/libxslt-bin
touch %{buildroot}/usr/share/doc/libxslt-bin/dummy
mkdir -p %{buildroot}/usr/share/doc/docbook-xml
touch %{buildroot}/usr/share/doc/docbook-xml/dummy
mkdir -p %{buildroot}/usr/share/doc/elfutils-dev
touch %{buildroot}/usr/share/doc/elfutils-dev/dummy

%files
%dir /usr/share/doc/attr-dev
/usr/share/doc/attr-dev/dummy
%files -n automake-dev
%dir /usr/share/doc/automake-dev
/usr/share/doc/automake-dev/dummy
%files -n glib-dev
%dir /usr/share/doc/glib-dev
/usr/share/doc/glib-dev/dummy
%files -n libcap-dev
%dir /usr/share/doc/libcap-dev
/usr/share/doc/libcap-dev/dummy
%files -n libcap-ng-dev
%dir /usr/share/doc/libcap-ng-dev
/usr/share/doc/libcap-ng-dev/dummy
%files -n libtool-dev
%dir /usr/share/doc/libtool-dev
/usr/share/doc/libtool-dev/dummy
%files -n numactl-dev
%dir /usr/share/doc/numactl-dev
/usr/share/doc/numactl-dev/dummy
%files -n python-dev
%dir /usr/share/doc/python-dev
/usr/share/doc/python-dev/dummy
%files -n zlib-dev
%dir /usr/share/doc/zlib-dev
/usr/share/doc/zlib-dev/dummy
%files -n libpng-dev
%dir /usr/share/doc/libpng-dev
/usr/share/doc/libpng-dev/dummy
%files -n gtk-doc-dev
%dir /usr/share/doc/gtk-doc-dev
/usr/share/doc/gtk-doc-dev/dummy
%files -n libxml2-dev
%dir /usr/share/doc/libxml2-dev
/usr/share/doc/libxml2-dev/dummy
%files -n libxslt-bin
%dir /usr/share/doc/libxslt-bin
/usr/share/doc/libxslt-bin/dummy
%files -n docbook-xml
%dir /usr/share/doc/docbook-xml
/usr/share/doc/docbook-xml/dummy
%files -n elfutils-dev
%dir /usr/share/doc/elfutils-dev
/usr/share/doc/elfutils-dev/dummy

