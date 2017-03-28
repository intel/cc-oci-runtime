#!/bin/bash

in_file="${1}"
out_file="${2}"
SED="$(which sed)"
if [ $? -ne 0 ]; then
	SED=/bin/sed
fi

${SED} \
	-e 's|@bindir@|'"${bindir}"'|g' \
	-e 's|@srcdir@|'"${srcdir}"'|g' \
	-e 's|@libexecdir@|'"${libexecdir}"'|' \
	-e 's|@localstatedir@|'"${localstatedir}"'|g' \
	-e 's|@BUNDLE_TEST_PATH@|'"${BUNDLE_TEST_PATH}"'|g' \
	-e 's|@CMDLINE@|'"${CMDLINE}"'|g' \
	-e 's|@CONTAINER_KERNEL@|'"${CONTAINER_KERNEL}"'|g' \
	-e 's|@CONTAINERS_IMG@|'"${CONTAINERS_IMG}"'|g' \
	-e 's|@DEFAULTSDIR@|'"${DEFAULTSDIR}"'|g' \
	-e 's|@PACKAGE_NAME@|'"${PACKAGE_NAME}"'|g' \
	-e 's|@QEMU_PATH@|'"${QEMU_PATH}"'|g' \
	-e 's|@BATS_PATH@|'"${BATS_PATH}"'|g' \
	-e 's|@ROOTFS_PATH@|'"${ROOTFS_PATH}"'|g' \
	-e 's|@SYSCONFDIR@|'"${SYSCONFDIR}"'|g' \
	-e 's|@DOCKER_FEDORA_VERSION@|'"${DOCKER_FEDORA_VERSION}"'|g' \
	-e 's|@DOCKER_ENGINE_FEDORA_VERSION@|'"${DOCKER_ENGINE_FEDORA_VERSION}"'|g' \
	-e 's|@DOCKER_UBUNTU_VERSION@|'"${DOCKER_UBUNTU_VERSION}"'|g' \
	-e 's|@DOCKER_ENGINE_UBUNTU_VERSION@|'"${DOCKER_ENGINE_UBUNTU_VERSION}"'|g' \
	-e 's|@CRIO_CACHE@|'"${CRIO_CACHE}"'|g' \
	-e 's|@ABS_BUILDDIR@|'"${abs_builddir}"'|g' \
	"${in_file}" > "${out_file}"
