/*
 * This file is part of clr-oci-runtime.
 * 
 * Copyright (C) 2016 Intel Corporation
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <check.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "../src/oci.h"
#include "../src/namespace.h"
#include "../src/util.h"
#include "../src/logging.h"
#include "test_common.h"

gboolean enable_debug = true;

START_TEST(test_clr_oci_ns_to_str) {
	const char *str;

	/* invalid enum values */
	ck_assert (! clr_oci_ns_to_str (999999));
	ck_assert (! clr_oci_ns_to_str (123456));

	str = clr_oci_ns_to_str (OCI_NS_CGROUP);
	ck_assert (! g_strcmp0 (str, "cgroup"));

	str = clr_oci_ns_to_str (OCI_NS_IPC);
	ck_assert (! g_strcmp0 (str, "ipc"));

	str = clr_oci_ns_to_str (OCI_NS_MOUNT);
	ck_assert (! g_strcmp0 (str, "mount"));

	str = clr_oci_ns_to_str (OCI_NS_NET);
	ck_assert (! g_strcmp0 (str, "network"));

	str = clr_oci_ns_to_str (OCI_NS_PID);
	ck_assert (! g_strcmp0 (str, "pid"));

	str = clr_oci_ns_to_str (OCI_NS_USER);
	ck_assert (! g_strcmp0 (str, "user"));

	str = clr_oci_ns_to_str (OCI_NS_UTS);
	ck_assert (! g_strcmp0 (str, "uts"));

} END_TEST

START_TEST(test_clr_oci_str_to_ns) {

	ck_assert (clr_oci_str_to_ns (NULL) == OCI_NS_INVALID);
	ck_assert (clr_oci_str_to_ns ("") == OCI_NS_INVALID);
	ck_assert (clr_oci_str_to_ns ("foo bar") == OCI_NS_INVALID);
	ck_assert (clr_oci_str_to_ns ("cgroup") == OCI_NS_CGROUP);
	ck_assert (clr_oci_str_to_ns ("ipc") == OCI_NS_IPC);
	ck_assert (clr_oci_str_to_ns ("mount") == OCI_NS_MOUNT);
	ck_assert (clr_oci_str_to_ns ("network") == OCI_NS_NET);
	ck_assert (clr_oci_str_to_ns ("pid") == OCI_NS_PID);
	ck_assert (clr_oci_str_to_ns ("user") == OCI_NS_USER);
	ck_assert (clr_oci_str_to_ns ("uts") == OCI_NS_UTS);

} END_TEST

START_TEST(test_clr_oci_ns_setup) {

	struct clr_oci_config config = { 0 };
	struct oci_cfg_namespace *ns = NULL;

	ck_assert (! clr_oci_ns_setup (NULL));

	/* no namespaces, so successful (NOP) setup */
	ck_assert (! config.oci.oci_linux.namespaces);
	ck_assert (clr_oci_ns_setup (&config));

	ns = g_malloc0 (sizeof (struct oci_cfg_namespace));
	ck_assert (ns);

	config.oci.oci_linux.namespaces = g_slist_append (config.oci.oci_linux.namespaces, ns);

	/* implicitly invalid namespaces are ignored */
	ck_assert (clr_oci_ns_setup (&config));

	ns->type = OCI_NS_INVALID;

	/* explicitly invalid namespaces are ignored */
	ck_assert (clr_oci_ns_setup (&config));

	/* most namespaces are silently ignored */
	ns->type = OCI_NS_CGROUP;
	ck_assert (clr_oci_ns_setup (&config));

	ns->type = OCI_NS_IPC;
	ck_assert (clr_oci_ns_setup (&config));

	ns->type = OCI_NS_MOUNT;
	ck_assert (clr_oci_ns_setup (&config));

	ns->type = OCI_NS_PID;
	ck_assert (clr_oci_ns_setup (&config));

	ns->type = OCI_NS_USER;
	ck_assert (clr_oci_ns_setup (&config));

	ns->type = OCI_NS_UTS;
	ck_assert (clr_oci_ns_setup (&config));

	/* net namespaces are honoured, but only run the tests
	 * as a non-priv user just in case.
	 */
	if (getuid ()) {
		int saved;
		gchar *tmpdir = g_dir_make_tmp (NULL, NULL);
		ns->type = OCI_NS_NET;
		ck_assert (! clr_oci_ns_setup (&config));

		/* unshare(2) error */
		ck_assert (errno == EPERM);

		/* set path so setns(2) gets called */
		ns->path = g_build_path ("/", tmpdir, "ns", NULL);
		ck_assert (ns->path);
		ck_assert (g_file_set_contents (ns->path, "", -1, NULL));

		ck_assert (! clr_oci_ns_setup (&config));
		/* setns(2) error */
		saved = errno;

		/* Normally EINVAL is returned, but if run under
		 * valgrind, the error could be ENOSYS since valgrind
		 * doesn't implement this syscall seemingly.
		 */
		ck_assert (saved == EINVAL || saved == ENOSYS);

		ck_assert (! g_remove (ns->path));
		g_free (ns->path);

		ns->path = g_strdup ("/proc/self/ns/net");
		ck_assert (ns->path);

		/* now we're passing a valid ns path, but non-priv users
		 * can't call setns due to insufficient privs.
		 */
		ck_assert (! clr_oci_ns_setup (&config));
		ck_assert (errno = EPERM);

		ck_assert (! g_remove (tmpdir));

		g_free (tmpdir);
	}

	/* clean up */
	clr_oci_config_free (&config);

} END_TEST

Suite* make_ns_suite (void) {
	Suite* s = suite_create(__FILE__);

	ADD_TEST(test_clr_oci_ns_to_str, s);
	ADD_TEST(test_clr_oci_str_to_ns, s);
	ADD_TEST(test_clr_oci_ns_setup, s);

	return s;
}

int main (void) {
	int number_failed;
	Suite* s;
	SRunner* sr;
	struct clr_log_options options = { 0 };

	options.use_json = false;
	options.filename = g_strdup ("namespace_test_debug.log");
	(void)clr_oci_log_init(&options);

	s = make_ns_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	clr_oci_log_free (&options);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
