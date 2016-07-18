#  This file is part of clr-oci-runtime.
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

CLEANFILES = *.log

ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = parallel-tests

CHECK_DEPS =

GIT_COMMIT := $(shell git rev-parse HEAD 2>/dev/null)

commit_id:
	if [ "x$(GIT_COMMIT)" != "x" ]; then \
		echo "Writing commit id"; \
		echo $(GIT_COMMIT) > commit_id; \
	else \
		echo "WARNING: cannot commit id"; \
		echo "0" > commit_id; \
	fi

STORED_COMMIT = $(shell cat commit_id 2>/dev/null)

src/commands/clr_oci_runtime-version.$(OBJEXT): \
	 commit_id

AM_CPPFLAGS = -I $(top_srcdir)/src -DG_LOG_DOMAIN=\"$(PACKAGE_NAME)\" \
	-DSYSCONFDIR=\"$(sysconfdir)/$(PACKAGE_NAME)\" \
	-DDEFAULTSDIR=\"$(datadir)/defaults/$(PACKAGE_NAME)\" \
	-DGIT_COMMIT=\"$(STORED_COMMIT)\"

defaultsdir = $(datadir)/defaults/$(PACKAGE_NAME)
defaults_DATA = data/vm.json data/hypervisor.args data/kernel-cmdline

AM_CFLAGS = -std=gnu99 -fstack-protector -Wall -pedantic \
	-Wstrict-prototypes -Wundef -fno-common \
	-Werror-implicit-function-declaration \
	-Wformat -Wformat-security -Werror=format-security \
	-Wconversion -Wunreachable-code

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-debug \
	--enable-tests \
	--enable-cppcheck \
	--enable-valgrind \
	--enable-code-coverage \
	--enable-functional-tests

bin_PROGRAMS = clr-oci-runtime

common_sources = \
	src/util.c src/util.h \
	src/logging.c src/logging.h \
	src/oci.c src/oci.h \
	src/process.c src/process.h \
	src/mount.c src/mount.h \
	src/network.c src/network.h \
	src/state.c src/state.h \
	src/events.c src/events.h \
	src/runtime.c src/runtime.h \
	src/semver.c src/semver.h \
	src/annotation.c src/annotation.h \
	src/namespace.c src/namespace.h \
	src/priv.c src/priv.h \
	src/oci-config.c src/oci-config.h \
	src/hypervisor.c src/hypervisor.h \
	src/json.c src/json.h \
	src/spec_handler.c src/spec_handler.h \
	src/common.h \
	src/command.c src/command.h \
	src/commands/attach.c \
	src/commands/create.c \
	src/commands/delete.c \
	src/commands/exec.c \
	src/commands/events.c \
	src/commands/help.c \
	src/commands/kill.c \
	src/commands/list.c \
	src/commands/run.c \
	src/commands/start.c \
	src/commands/state.c \
	src/commands/stop.c \
	src/commands/pause.c \
	src/commands/ps.c \
	src/commands/resume.c \
	src/commands/version.c \
	src/commands/checkpoint.c \
	src/commands/restore.c \
	src/commands/update.c \
	src/spec_handlers/hooks.c \
	src/spec_handlers/platform.c \
	src/spec_handlers/process.c \
	src/spec_handlers/root.c \
	src/spec_handlers/vm.c \
	src/spec_handlers/linux.c \
	src/spec_handlers/mounts.c \
	src/spec_handlers/annotations.c

clr_oci_runtime_SOURCES = \
	src/main.c \
	$(common_sources)

clr_oci_runtime_LDADD = \
	$(GLIB_LIBS) \
	$(GIO_LIBS) \
	$(JSON_GLIB_LIBS) \
	$(UUID_LIBS)

clr_oci_runtime_CFLAGS = \
	$(AM_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
	$(JSON_GLIB_CFLAGS) \
	$(UUID_CFLAGS)

bats_test_sources = \
	tests/functional/README \
	tests/functional/README.rst \
	tests/functional/common.bash.in \
	tests/functional/help.bats \
	tests/functional/start.bats \
	tests/functional/version.bats \
	tests/functional/data/config-minimal-clr-oci.json.in

EXTRA_DIST = \
	LICENSE \
	TODO \
	CONTRIBUTING.md \
	Doxyfile \
	data/vm.json.in \
	tests/data \
	commit_id \
	$(defaults_DATA) \
	$(bats_test_sources)

if CPPCHECK
CHECK_DEPS += cppcheck
cppcheck:
	@$(CPPCHECK_PATH) --enable=performance,unusedFunction,missingInclude \
		--language=c --std=c99 --std=posix \
		--error-exitcode=1 -I$(srcdir)/src $(srcdir)/src
endif

if FUNCTIONAL_TEST
CHECK_DEPS += functional-test
functional-test:
	@$(BATS_PATH) $(srcdir)/tests/functional/
endif

#### tests ####
if BUILD_TESTS

if VALGRIND_ENABLED
@VALGRIND_CHECK_RULES@
VALGRIND_memcheck_FLAGS = --tool=memcheck --error-exitcode=1 --leak-check=full --errors-for-leak-kinds=definite,indirect --show-possibly-lost=no
CHECK_DEPS += check-valgrind
endif

if CODE_COVERAGE_ENABLED
@CODE_COVERAGE_RULES@
CHECK_DEPS += code-coverage-capture
endif

check_LTLIBRARIES = libtest.la

libtest_la_SOURCES = \
	$(common_sources)

libtest_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(CODE_COVERAGE_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
	$(JSON_GLIB_CFLAGS) \
	$(UUID_CFLAGS) \
	-DUNIT_TESTING

libtest_la_LIBS = \
	$(CODE_COVERAGE_LDFLAGS)

TEST_COMMON_CFLAGS = \
	$(AM_CFLAGS) \
	$(CODE_COVERAGE_CFLAGS) \
	$(JSON_GLIB_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(CHECK_CFLAGS) \
	-std=gnu99 \
	-DTEST_DATA_DIR=\"$(abs_srcdir)/tests/data\" \
	-DUNIT_TESTING

TEST_COMMON_LDADD = \
	libtest.la \
	$(CODE_COVERAGE_LDFLAGS) \
	$(JSON_GLIB_LIBS) \
	$(GLIB_LIBS) \
	$(CHECK_LIBS) \
	$(UUID_LIBS)

TEST_COMMON_SOURCES = \
	tests/test_common.c \
	tests/test_common.h

TESTS = \
	hypervisor_test \
	json_test \
	logging_test \
	namespace_test \
	oci_config_test \
	oci_test \
	priv_test \
	process_test \
	runtime_test \
	semver_test \
	state_test \
	util_test \
	mount_test \
	annotation_test \
	sh_annotations_test \
	sh_linux_test \
	sh_vm_test \
	sh_platform_test \
	sh_process_test \
	sh_root_test \
	sh_mounts_test \
	sh_hooks_test

noinst_PROGRAMS = \
	$(TESTS)

check_PROGRAMS = \
	$(TESTS)

## hypervisor.c test ##
hypervisor_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/hypervisor_test.c

hypervisor_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

hypervisor_test_LDADD = \
	$(TEST_COMMON_LDADD)

## json.c test ##
json_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/json_test.c

json_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

json_test_LDADD = \
	$(TEST_COMMON_LDADD)

## logging.c test ##
logging_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/logging_test.c

logging_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

logging_test_LDADD = \
	$(TEST_COMMON_LDADD)

## namespace.c test ##
namespace_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/namespace_test.c

namespace_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

namespace_test_LDADD = \
	$(TEST_COMMON_LDADD)

## oci-config.c test ##
oci_config_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/oci-config_test.c

oci_config_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

oci_config_test_LDADD = \
	$(TEST_COMMON_LDADD)

## oci.c test ##
oci_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/oci_test.c

oci_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

oci_test_LDADD = \
	$(TEST_COMMON_LDADD)

## process.c test ##
process_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/process_test.c

process_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

process_test_LDADD = \
	$(TEST_COMMON_LDADD)

## runtime.c test ##
runtime_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/runtime_test.c

runtime_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

runtime_test_LDADD = \
	$(TEST_COMMON_LDADD)

## semver.c test ##
semver_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/semver_test.c

semver_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

semver_test_LDADD = \
	$(TEST_COMMON_LDADD)

## state.c test ##
state_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/state_test.c

state_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

state_test_LDADD = \
	$(TEST_COMMON_LDADD)

## util.c test ##
util_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/util_test.c

util_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

util_test_LDADD = \
	$(TEST_COMMON_LDADD)

## priv.c test ##
priv_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/priv_test.c

priv_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

priv_test_LDADD = \
	$(TEST_COMMON_LDADD)

## mount.c test ##
mount_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/mount_test.c

mount_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

mount_test_LDADD = \
	$(TEST_COMMON_LDADD)

## annotation.c test ##
annotation_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/annotation_test.c

annotation_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

annotation_test_LDADD = \
	$(TEST_COMMON_LDADD)

## spec_handlers/annotations.c test ##
sh_annotations_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/spec_handlers/annotations_test.c

sh_annotations_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

sh_annotations_test_LDADD = \
	$(TEST_COMMON_LDADD)

## spec_handlers/linux.c test ##
sh_linux_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/spec_handlers/linux_test.c

sh_linux_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

sh_linux_test_LDADD = \
	$(TEST_COMMON_LDADD)

## spec_handlers/vm.c test ##
sh_vm_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/spec_handlers/vm_test.c

sh_vm_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

sh_vm_test_LDADD = \
	$(TEST_COMMON_LDADD)

## spec_handlers/platform.c test ##
sh_platform_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/spec_handlers/platform_test.c

sh_platform_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

sh_platform_test_LDADD = \
	$(TEST_COMMON_LDADD)

## spec_handlers/process.c test ##
sh_process_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/spec_handlers/process_test.c

sh_process_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

sh_process_test_LDADD = \
	$(TEST_COMMON_LDADD)

## spec_handlers/root.c test ##
sh_root_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/spec_handlers/root_test.c

sh_root_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

sh_root_test_LDADD = \
	$(TEST_COMMON_LDADD)

## spec_handlers/mounts.c test ##
sh_mounts_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/spec_handlers/mounts_test.c

sh_mounts_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

sh_mounts_test_LDADD = \
	$(TEST_COMMON_LDADD)

## spec_handlers/hooks.c test ##
sh_hooks_test_SOURCES = \
	$(TEST_COMMON_SOURCES) \
	tests/spec_handlers/hooks_test.c

sh_hooks_test_CFLAGS = \
	$(TEST_COMMON_CFLAGS)

sh_hooks_test_LDADD = \
	$(TEST_COMMON_LDADD)

CLEANFILES += tests/*~ tests/*.log tests/*.trs
CLEANFILES += core core.* vgcore.*
endif

check: $(CHECK_DEPS)

clean-local:
	rm -f commit_id
