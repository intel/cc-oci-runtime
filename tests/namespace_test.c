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

#include <check.h>
#include <glib.h>

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

Suite* make_ns_suite (void) {
	Suite* s = suite_create(__FILE__);

	ADD_TEST(test_clr_oci_ns_to_str, s);
	ADD_TEST(test_clr_oci_str_to_ns, s);

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
