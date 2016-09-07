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
gboolean cc_run_hook (struct oci_cfg_hook* hook,
		const gchar* state,
		gsize state_length);

extern GMainLoop *hook_loop;

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

START_TEST(test_cc_run_hook) {

	struct oci_cfg_hook *hook = NULL;
	g_autofree gchar *cmd = NULL;

	/* XXX: note that the command the tests run must read from
	 * stdin!
	 *
	 * dd was chosen since not only does it read from stdin, but
	 * also accepts arguments that can be tested, and those
	 * arguments can be repeated without generating an error.
	 */
	cmd = g_find_program_in_path ("dd");
	ck_assert (cmd);

	/*******************************/

	hook_loop = g_main_loop_new (NULL, 0);
	ck_assert (hook_loop);

	hook = g_new0 (struct oci_cfg_hook, 1);
	ck_assert (hook);

	g_strlcpy (hook->path, "dd", sizeof (hook->path));

	/* fails since full path not specified */
	ck_assert (! cc_run_hook (hook, "", 1));

	g_main_loop_unref (hook_loop);
	cc_oci_hook_free (hook);

	/*******************************/
	/* full path specified, no args */

	hook_loop = g_main_loop_new (NULL, 0);
	ck_assert (hook_loop);

	hook = g_new0 (struct oci_cfg_hook, 1);
	ck_assert (hook);

	g_strlcpy (hook->path, cmd, sizeof (hook->path));

	ck_assert (cc_run_hook (hook, "", 1));

	g_main_loop_unref (hook_loop);
	cc_oci_hook_free (hook);

	/*******************************/
	/* full path specified, cmd arg */

	hook_loop = g_main_loop_new (NULL, 0);
	ck_assert (hook_loop);

	hook = g_new0 (struct oci_cfg_hook, 1);
	ck_assert (hook);

	g_strlcpy (hook->path, cmd, sizeof (hook->path));

	hook->args = g_new0 (gchar *, 2);
	ck_assert (hook->args);
	hook->args[0] = g_strdup (cmd);

	ck_assert (cc_run_hook (hook, "", 1));

	g_main_loop_unref (hook_loop);
	cc_oci_hook_free (hook);

	/*******************************/
	/* full path, cmd arg + 1 arg */

	hook_loop = g_main_loop_new (NULL, 0);
	ck_assert (hook_loop);

	hook = g_new0 (struct oci_cfg_hook, 1);
	ck_assert (hook);

	g_strlcpy (hook->path, cmd, sizeof (hook->path));

	hook->args = g_new0 (gchar *, 3);
	ck_assert (hook->args);
	hook->args[0] = g_strdup (cmd);
	hook->args[1] = g_strdup ("bs=1");

	ck_assert (cc_run_hook (hook, "", 1));

	g_main_loop_unref (hook_loop);
	cc_oci_hook_free (hook);

	/*******************************/
	/* full path, cmd arg + 2 args */

	hook_loop = g_main_loop_new (NULL, 0);
	ck_assert (hook_loop);

	hook = g_new0 (struct oci_cfg_hook, 1);
	ck_assert (hook);

	g_strlcpy (hook->path, cmd, sizeof (hook->path));

	hook->args = g_new0 (gchar *, 4);
	ck_assert (hook->args);
	hook->args[0] = g_strdup (cmd);
	hook->args[1] = g_strdup ("bs=1");
	hook->args[2] = g_strdup ("bs=1");

	ck_assert (cc_run_hook (hook, "", 1));

	g_main_loop_unref (hook_loop);
	cc_oci_hook_free (hook);

	/*******************************/
	/* full path, cmd arg + 3 args */

	hook_loop = g_main_loop_new (NULL, 0);
	ck_assert (hook_loop);

	hook = g_new0 (struct oci_cfg_hook, 1);
	ck_assert (hook);

	g_strlcpy (hook->path, cmd, sizeof (hook->path));

	hook->args = g_new0 (gchar *, 5);
	ck_assert (hook->args);
	hook->args[0] = g_strdup (cmd);
	hook->args[1] = g_strdup ("bs=1");
	hook->args[2] = g_strdup ("bs=1");
	hook->args[3] = g_strdup ("bs=1");

	ck_assert (cc_run_hook (hook, "", 1));

	g_main_loop_unref (hook_loop);
	cc_oci_hook_free (hook);

	/*******************************/

} END_TEST

Suite* make_process_suite(void) {
	Suite* s = suite_create(__FILE__);

	ADD_TEST(test_cc_oci_cmd_is_shell, s);
	ADD_TEST(test_cc_run_hook, s);

	return s;
}

int main(void) {
	int number_failed;
	Suite* s;
	SRunner* sr;
	struct cc_log_options options = { 0 };

	options.enable_debug = true;
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
