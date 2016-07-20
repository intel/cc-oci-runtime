/*
 * This file is part of cc-oci-runtime.
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

#include "test_common.h"
#include "../src/logging.h"
#include "../src/process.h"

gboolean cc_oci_cmd_is_shell (const char *cmd);

START_TEST(test_cc_oci_cmd_is_shell) {

	ck_assert (! cc_oci_cmd_is_shell (""));
	ck_assert (! cc_oci_cmd_is_shell (NULL));

	ck_assert (cc_oci_cmd_is_shell ("sh"));
	ck_assert (cc_oci_cmd_is_shell ("/sh"));
	ck_assert (cc_oci_cmd_is_shell ("/bin/sh"));
	ck_assert (cc_oci_cmd_is_shell ("/usr/bin/sh"));
	ck_assert (cc_oci_cmd_is_shell ("/usr/local/bin/sh"));

	ck_assert (cc_oci_cmd_is_shell ("bash"));
	ck_assert (cc_oci_cmd_is_shell ("/bash"));
	ck_assert (cc_oci_cmd_is_shell ("/bin/bash"));
	ck_assert (cc_oci_cmd_is_shell ("/usr/bin/bash"));
	ck_assert (cc_oci_cmd_is_shell ("/usr/local/bin/bash"));

	ck_assert (cc_oci_cmd_is_shell ("zsh"));
	ck_assert (cc_oci_cmd_is_shell ("/zsh"));
	ck_assert (cc_oci_cmd_is_shell ("/bin/zsh"));
	ck_assert (cc_oci_cmd_is_shell ("/usr/bin/zsh"));
	ck_assert (cc_oci_cmd_is_shell ("/usr/local/bin/zsh"));

	ck_assert (cc_oci_cmd_is_shell ("ksh"));
	ck_assert (cc_oci_cmd_is_shell ("/ksh"));
	ck_assert (cc_oci_cmd_is_shell ("/bin/ksh"));
	ck_assert (cc_oci_cmd_is_shell ("/usr/bin/ksh"));
	ck_assert (cc_oci_cmd_is_shell ("/usr/local/bin/ksh"));

	ck_assert (cc_oci_cmd_is_shell ("csh"));
	ck_assert (cc_oci_cmd_is_shell ("/csh"));
	ck_assert (cc_oci_cmd_is_shell ("/bin/csh"));
	ck_assert (cc_oci_cmd_is_shell ("/usr/bin/csh"));
	ck_assert (cc_oci_cmd_is_shell ("/usr/local/bin/csh"));

	ck_assert (! cc_oci_cmd_is_shell ("true"));
	ck_assert (! cc_oci_cmd_is_shell ("/true"));
	ck_assert (! cc_oci_cmd_is_shell ("/bin/true"));
	ck_assert (! cc_oci_cmd_is_shell ("/usr/bin/true"));
	ck_assert (! cc_oci_cmd_is_shell ("/usr/local/bin/true"));

} END_TEST

Suite* make_process_suite(void) {
	Suite* s = suite_create(__FILE__);

	ADD_TEST(test_cc_oci_cmd_is_shell, s);

	return s;
}

gboolean enable_debug = true;

int main(void) {
	int number_failed;
	Suite* s;
	SRunner* sr;
	struct cc_log_options options = { 0 };

	options.use_json = false;
	options.filename = g_strdup ("process_test_debug.log");
	(void)cc_oci_log_init(&options);

	s = make_process_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	cc_oci_log_free (&options);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
