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

#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <glib.h>
#include <uuid/uuid.h>

#include "oci.h"
#include "util.h"
#include "hypervisor.h"
#include "common.h"

/** Length of an ASCII-formatted UUID */
#define UUID_MAX 37

/* Values passed in from automake.
 *
 * XXX: They are assigned to variables to allow the tests
 * to modify the values.
 */
private gchar *sysconfdir = SYSCONFDIR;
private gchar *defaultsdir = DEFAULTSDIR;

/*!
 * Replace any special tokens found in \p args with their expanded
 * values.
 *
 * \param config \ref cc_oci_config.
 * \param[in, out] args Command-line to expand.
 *
 * \warning this is not very efficient.
 *
 * \return \c true on success, else \c false.
 */
private gboolean
cc_oci_expand_cmdline (struct cc_oci_config *config,
		gchar **args)
{
	struct stat       st;
	gchar           **arg;
	gchar            *bytes = NULL;
	gchar            *console_device = NULL;
	g_autofree gchar *procsock_device = NULL;

	gboolean          ret = false;
	gint              count;
	uuid_t            uuid;
	/* uuid pattern */
	const char        uuid_pattern[UUID_MAX] = "00000000-0000-0000-0000-000000000000";
	char              uuid_str[UUID_MAX] = { 0 };
	gint              uuid_index = 0;

	if (! (config && args)) {
		return false;
	}

	if (! config->vm) {
		g_critical ("No vm configuration");
		goto out;
	}

	/* We're about to launch the hypervisor so validate paths.*/

	if ((!config->vm->image_path[0])
		|| stat (config->vm->image_path, &st) < 0) {
		g_critical ("image file: %s does not exist",
			    config->vm->image_path);
		return false;
	}

	if (!(config->vm->kernel_path[0]
		&& g_file_test (config->vm->kernel_path, G_FILE_TEST_EXISTS))) {
		g_critical ("kernel image: %s does not exist",
			    config->vm->kernel_path);
		return false;
	}

	if (!(config->oci.root.path[0]
		&& g_file_test (config->oci.root.path, G_FILE_TEST_EXISTS|G_FILE_TEST_IS_DIR))) {
		g_critical ("workload directory: %s does not exist",
			    config->oci.root.path);
		return false;
	}

	uuid_generate_random(uuid);
	for(size_t i=0; i<sizeof(uuid_t) && uuid_index < sizeof(uuid_pattern); ++i) {
		/* hex to char */
		uuid_index += g_snprintf(uuid_str+uuid_index,
		                  sizeof(uuid_pattern)-(gulong)uuid_index,
		                  "%02x", uuid[i]);

		/* copy separator '-' */
		if (uuid_pattern[uuid_index] == '-') {
			uuid_index += g_snprintf(uuid_str+uuid_index,
			                  sizeof(uuid_pattern)-(gulong)uuid_index, "-");
		}
	}

	bytes = g_strdup_printf ("%lu", (unsigned long int)st.st_size);

	/* XXX: Note that "signal=off" ensures that the key sequence
	 * CONTROL+c will not cause the VM to exit.
	 */
	if (! config->console || ! g_utf8_strlen(config->console, LINE_MAX)) {

		config->use_socket_console = true;

		/* Temporary fix for non-console output, since -chardev stdio is not working as expected
		 * 
		 * Check if called from docker. Use -chardev pipe as virtualconsole.
		 * Create symlinks to docker named pipes in the format qemu expects.
		 *
		 * Eventually move to using "stdio,id=charconsole0,signal=off"
		 */
		if (! isatty (STDIN_FILENO)) {

			config->console = g_build_path ("/",
					config->bundle_path,
					"cc-std", NULL);

			g_debug ("no console device provided , so using pipe: %s", config->console);

			g_autofree gchar *init_stdout = g_build_path ("/",
							config->bundle_path,
							"init-stdout", NULL);

			g_autofree gchar *cc_stdout = g_build_path ("/",
							config->bundle_path,
							"cc-std.out", NULL);

			g_autofree gchar *init_stdin = g_build_path ("/",
							config->bundle_path,
							"init-stdin", NULL);
			g_autofree gchar *cc_stdin = g_build_path ("/",
							config->bundle_path,
							"cc-std.in", NULL);

			if ( symlink (init_stdout, cc_stdout) == -1) {
				g_critical("Failed to create symlink for output pipe: %s",
					   strerror (errno));
				goto out;
			}

			if ( symlink (init_stdin, cc_stdin) == -1) {
				g_critical("Failed to create symlink for input pipe: %s",
					   strerror (errno));
				goto out;
			}

			console_device = g_strdup_printf ("pipe,id=charconsole0,path=%s", config->console);

		} else {

			/* In case the runtime is called standalone without console */

			/* No console specified, so make the hypervisor create
			 * a Unix domain socket.
			 */	
			config->console = g_build_path ("/",
					config->state.runtime_path,
					CC_OCI_CONSOLE_SOCKET, NULL);

			/* Note that path is not quoted - attempting to do so
			 * results in qemu failing with the error:
			 *
			 *   Failed to bind socket to "/a/dir/console.sock": No such file or directory
			 */

			g_debug ("no console device provided, so using socket: %s", config->console);

			console_device = g_strdup_printf ("socket,path=%s,server,nowait,id=charconsole0,signal=off",
				config->console);
		}
	} else {
		console_device = g_strdup ("stdio,id=charconsole0,signal=off");
	}

	procsock_device = g_strdup_printf ("socket,id=procsock,path=%s,server,nowait", config->state.procsock_path);

	for (arg = args, count = 0; arg && *arg; arg++, count++) {
		if (! count) {
			/* command must be the first entry */
			if (! g_path_is_absolute (*arg)) {
				gchar *cmd = g_find_program_in_path (*arg);

				if (cmd) {
					g_free (*arg);
					*arg = cmd;
				}
			}
		}

		ret = cc_oci_replace_string (arg, "@WORKLOAD_DIR@",
				config->oci.root.path);
		if (! ret) {
			goto out;
		}

		ret = cc_oci_replace_string (arg, "@KERNEL@",
				config->vm->kernel_path);
		if (! ret) {
			goto out;
		}

		ret = cc_oci_replace_string (arg, "@KERNEL_PARAMS@",
				config->vm->kernel_params);
		if (! ret) {
			goto out;
		}

		ret = cc_oci_replace_string (arg, "@IMAGE@",
				config->vm->image_path);
		if (! ret) {
			goto out;
		}

		ret = cc_oci_replace_string (arg, "@SIZE@", bytes);
		if (! ret) {
			goto out;
		}

		ret = cc_oci_replace_string (arg, "@COMMS_SOCKET@",
				config->state.comms_path);
		if (! ret) {
			goto out;
		}

		ret = cc_oci_replace_string (arg, "@PROCESS_SOCKET@",
				procsock_device);
		if (! ret) {
			goto out;
		}

		ret = cc_oci_replace_string (arg, "@CONSOLE_DEVICE@",
				console_device);
		if (! ret) {
			goto out;
		}

		ret = cc_oci_replace_string (arg, "@NAME@",
				g_strrstr(uuid_str, "-")+1);
		if (! ret) {
			goto out;
		}

		ret = cc_oci_replace_string (arg, "@UUID@", uuid_str);
		if (! ret) {
			goto out;
		}
	}

	ret = true;

out:
	g_free_if_set (bytes);
	g_free_if_set (console_device);

	return ret;
}

/*!
 * Determine the full path to the \ref CC_OCI_HYPERVISOR_CMDLINE_FILE
 * file.
 * Priority order to get file path : bundle dir, sysconfdir , defaultsdir
 *
 * \param config \ref cc_oci_config.
 *
 * \return Newly-allocated string on success, else \c NULL.
 */
private gchar *
cc_oci_vm_args_file_path (const struct cc_oci_config *config)
{
	gchar *args_file = NULL;

	if (! config) {
		return NULL;
	}

	if (! config->bundle_path) {
		return NULL;
	}

	args_file = cc_oci_get_bundlepath_file (config->bundle_path,
			CC_OCI_HYPERVISOR_CMDLINE_FILE);
	if (! args_file) {
		return NULL;
	}

	if (g_file_test (args_file, G_FILE_TEST_EXISTS)) {
		goto out;
	}

	g_free_if_set (args_file);

	/* Try sysconfdir if bundle file does not exist */
	args_file = g_build_path ("/", sysconfdir,
			CC_OCI_HYPERVISOR_CMDLINE_FILE, NULL);

	if (g_file_test (args_file, G_FILE_TEST_EXISTS)) {
		goto out;
	}

	g_free_if_set (args_file);

	/* Finally, try stateless dir */
	args_file = g_build_path ("/", defaultsdir,
			CC_OCI_HYPERVISOR_CMDLINE_FILE, NULL);

	if (g_file_test (args_file, G_FILE_TEST_EXISTS)) {
		goto out;
	}

	g_free_if_set (args_file);

	/* no file found, so give up */
	args_file = NULL;

out:
	g_debug ("using %s", args_file);
	return args_file;
}

/*!
 * Generate the list of hypervisor arguments to use.
 *
 * \param config \ref cc_oci_config.
 * \param[out] args Command-line to expand.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_vm_args_get (struct cc_oci_config *config,
		gchar ***args)
{
	gboolean  ret;
	gchar    *args_file = NULL;

	if (! (config && args)) {
		return false;
	}

	args_file = cc_oci_vm_args_file_path (config);
	if (! args_file) {
		g_critical("File %s not found",
				CC_OCI_HYPERVISOR_CMDLINE_FILE);
	}

	ret = cc_oci_file_to_strv (args_file, args);
	if (! ret) {
		goto out;
	}

	ret = cc_oci_expand_cmdline (config, *args);
	if (! ret) {
		goto out;
	}

	ret = true;
out:
	g_free_if_set (args_file);
	return ret;
}
