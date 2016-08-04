#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
#  This file is part of cc-oci-runtime.
#
#  Copyright (C) 2016 Intel Corporation
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

AC_PREREQ([2.69])
AC_INIT([cc-oci-runtime], [0.1], [dev@clearlinux.org])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror -Wno-portability silent-rules subdir-objects color-tests no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile data/vm.json data/config.json data/hypervisor.args tests/functional/common.bash tests/functional/data/config-minimal-cc-oci.json])
AC_LANG(C)

LT_INIT

AX_VALGRIND_DFLT([sgcheck], [off])
AX_VALGRIND_CHECK

AX_CODE_COVERAGE

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_MKDIR_P


# Checks for header files.
AC_CHECK_HEADERS([fcntl.h mntent.h stdlib.h string.h sys/mount.h unistd.h uuid/uuid.h gio/gio.h gio/gunixsocketaddress.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT64_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([atexit dup2 memset strdup])

AC_ARG_ENABLE(debug, AS_HELP_STRING([--enable-debug], [enable debug mode @<:@default=no@:>@]),
	      [], [enable_debug=no])
AS_IF([test "x$enable_debug" = "xyes"],
	[AC_DEFINE([DEBUG], [1], [Debugging mode enabled])],
	[AC_DEFINE([NDEBUG], [1], [Debugging and assertions disabled])])
AM_CONDITIONAL([DEBUG], [test x$enable_debug = x"yes"])

AS_IF([test "x$enable_debug" = "xyes"],
      [[CFLAGS=`echo "$CFLAGS" | sed -e "s/ -O[1-9s]*\b/ -O0/g"`
       CFLAGS=`echo "$CFLAGS" | sed -e "s/ -g\b/ -ggdb3/g"`
       LDFLAGS=`echo "$LDFLAGS" | sed -e "s/ -Wl,-O[0-9]*\b//g"`
       LDFLAGS=`echo "$LDFLAGS" | sed -e "s/ -g\b/ -ggdb3/g"`
      ]])

AC_ARG_ENABLE(tests, AS_HELP_STRING([--disable-tests], [do not build unit tests @<:@default=no@:>@]),
          [], [disable_tests=no])
AS_IF([test x"$disable_tests" = "xno"],
    [AC_SUBST([BUILD_TESTS], [1])],
    [AC_SUBST([BUILD_TESTS], [0])])
AM_CONDITIONAL([BUILD_TESTS], [test x$disable_tests = x"no"])

AC_ARG_ENABLE(cppcheck, AS_HELP_STRING([--enable-cppcheck], [enable static analysis @<:@default=no@:>@]),
          [], [enable_cppcheck=no])
AS_IF([test x"$enable_cppcheck" = "xyes"],
    [AC_SUBST([CPPCHECK], [1])],
    [AC_SUBST([CPPCHECK], [0])])
AM_CONDITIONAL([CPPCHECK], [test x$enable_cppcheck = x"yes"])

AC_ARG_ENABLE(functional-tests, AS_HELP_STRING([--disable-functional-tests], [disable functional tests @<:@default=no@:>@]),
          [], [disable_functional_test=no])
AS_IF([test x"$disable_functional_test" = "xno"],
    [AC_SUBST([FUNCTIONAL_TEST], [1])],
    [AC_SUBST([FUNCTIONAL_TEST], [0])])
AM_CONDITIONAL([FUNCTIONAL_TEST], [test x$disable_functional_test = x"no"])

# Checks for libraries.
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.44])
PKG_CHECK_MODULES([GIO], [gio-unix-2.0])
PKG_CHECK_MODULES([JSON_GLIB], [json-glib-1.0])
PKG_CHECK_MODULES([UUID], [uuid])

AS_IF([test $BUILD_TESTS = 1],
[PKG_CHECK_MODULES([CHECK], [check >= 0.9.14])]
)

AS_IF([test $CPPCHECK = 1],
   [AC_PATH_PROG([CPPCHECK_PATH], [cppcheck], [no], [$PATH:/sbin:/usr/sbin])
   if test x"$CPPCHECK_PATH" == x"no" ; then
       AC_MSG_ERROR([Please install cppcheck.])
   fi
   AC_SUBST(CPPCHECK_PATH)]
)

AS_IF([test $FUNCTIONAL_TEST = 1],
   [AC_PATH_PROG([BATS_PATH], [bats], [no], [$PATH:/sbin:/usr/sbin])
   if test x"$BATS_PATH" == x"no" ; then
       AC_MSG_ERROR([Please install bats (Bash Automated Testing System).)])
   fi
   AC_SUBST(BATS_PATH)]
)

#Check for qemu
DEFAULT_QEMU_PATH=/usr/bin/qemu-system-x86_64

AC_ARG_WITH([qemu-path],
    [AS_HELP_STRING([--with-qemu-path=[[QEMU-PATH]]],
        [path to qemu with pc-lite support  @<:@default=qemu-system-x86_64@:>@])
    ],
    [
        QEMU=$with_qemu_path
        QEMUout=`$QEMU -machine help | grep -o pc-lite | tail -1`
        AS_IF([test "x$QEMUout" != "xpc-lite"],
             AC_MSG_WARN([$QEMU does not support pc-lite machine])
        )
    ],
    [with_qemu_path=no]
)

# Check for netlink from iproute2
#AC_CHECK_HEADERS(libnetlink.h,  dummy=yes,
#    AC_MSG_ERROR(libnetlink header file from iproute2 is required))
#AC_SEARCH_LIBS(rtnl_close, netlink, dummy=yes,
#    AC_MSG_ERROR(netlink library support from iproute2 is required))

AC_CACHE_CHECK(
    [for qemu that supports pc-lite machine], [ac_cv_path_QEMU],
    [
        AC_PATH_PROGS_FEATURE_CHECK([QEMU],
            [qemu-lite-system-x86_64 qemu-system-x86_64],
            [
                QEMUout=`$ac_path_QEMU -machine help | grep -o pc-lite | tail -1`
                AS_IF([test "x$QEMUout" = "xpc-lite"],
                      [
                          ac_cv_path_QEMU=$ac_path_QEMU
                          has_pc_lite=yes
                          ac_path_QEMU_found=:
                      ])
            ],
            [
                AC_MSG_WARN([No qemu detected with pc-lite machine support using:])
                ac_cv_path_QEMU=$DEFAULT_QEMU_PATH
                has_pc_lite=no
            ]
        )
    ]
)

AC_SUBST([QEMU_PATH], [$ac_cv_path_QEMU])

#Read Kernel parameters
AC_SUBST([CMDLINE], [$(cat $srcdir/data/kernel-cmdline)])

#bundle path for functional-test
DEFAULT_BUNDLE_TEST_DIR=$(realpath $srcdir/tests/functional/bundle)
AC_ARG_WITH([tests-bundle-path],
    [AS_HELP_STRING([--with-tests-bundle-path=[[BUNDLE-PATH]]],
            [path to oci bundle to execute functional tests])
    ],
    [
        BUNDLE_TEST_PATH=$(realpath $with_tests_bundle_path)
        AC_SUBST([BUNDLE_TEST_PATH])
    ],
    [
        AC_SUBST([BUNDLE_TEST_PATH], [$DEFAULT_BUNDLE_TEST_DIR])
    ]
)

DEFAULT_ROOTFS_PATH=$BUNDLE_TEST_PATH/rootfs
AC_ARG_WITH([tests-rootfs-path],
    [AS_HELP_STRING([--with-tests-rootfs-path=[[ROOTFS-PATH]]],
        [path to rootfs for functional tests])
    ],
    [
        ROOTFS_PATH=$(realpath $with_tests_rootfs_path)
        AC_SUBST([ROOTFS_PATH])
    ],
    [
        AC_SUBST([ROOTFS_PATH], [$DEFAULT_ROOTFS_PATH])
    ]
)

#Kernel path for Clear Containers
DEFAULT_KERNEL_PATH=$localstatedir/lib/$PACKAGE_NAME/data/kernel/vmlinux.container
AC_ARG_WITH([cc-kernel],
    [AS_HELP_STRING([--with-cc-kernel=[[KERNEL-PATH]]],
        [clear container kernel path])
    ],
    [
        CONTAINER_KERNEL=$(realpath $with_cc_kernel)
        AC_SUBST([CONTAINER_KERNEL])
    ],
    [
        AC_SUBST([CONTAINER_KERNEL], [$DEFAULT_KERNEL_PATH])
    ]
)

#Container image path for Clear Containers
DEFAULT_CONTAINER_IMG=$localstatedir/lib/$PACKAGE_NAME/data/image/clear-containers.img
AC_ARG_WITH([cc-image],
    [AS_HELP_STRING([--with-cc-image=[[IMAGE-PATH]]],
        [clear container image path])
    ],
    [
        CONTAINERS_IMG=$(realpath $with_cc_image)
        AC_SUBST([CONTAINERS_IMG])
    ],
    [
        AC_SUBST([CONTAINERS_IMG],[$DEFAULT_CONTAINER_IMG])
    ]
)

AC_OUTPUT

dnl === Summary ===============================================================

AS_IF([test x"$disable_tests" = xno], [enable_tests=yes], [enable_tests=no])
AS_IF([test x"$disable_functional_test" = xno],
      [enable_functional_tests=yes], [enable_functional_tests=no])

AC_MSG_RESULT([
cc-oci-runtime - $VERSION

 • General:

       QEMU             : $ac_cv_path_QEMU
       QEMU pc-lite     : $has_pc_lite
       Container kernel : $CONTAINER_KERNEL
       Container image  : $CONTAINERS_IMG
       Debug build      : $enable_debug

 • Tests:

       Unit tests       : $enable_tests
       Functional tests : $enable_functional_tests
       Cppcheck         : $enable_cppcheck
       Valgrind         : $VALGRIND_ENABLED
       Code coverage    : $CODE_COVERAGE_ENABLED
])
