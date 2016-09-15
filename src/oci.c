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

/**
 * \file
 *
 * Open Container Initiative (OCI) routines.
 *
 * \see https://www.opencontainers.org/
 */

#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <gio/gunixsocketaddress.h>
#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

#include "common.h"
#include "oci.h"
#include "util.h"
#include "process.h"
#include "network.h"
#include "json.h"
#include "mount.h"
#include "state.h"
#include "oci-config.h"
#include "runtime.h"
#include "spec_handler.h"
#include "command.h"

extern struct start_data start_data;

/** Format options for VM fields to display. */
struct format_options
{
	/** If \c true, output in JSON format. */
	gboolean    use_json;

	/** Used for JSON formatting. */
	JsonArray  *array;

	/* If \c true, show hypervisor, image and kernel details. */
	gboolean    show_all;

	int         id_width;
	int         pid_width;
	int         status_width;
	int         bundle_width;
	int         created_width;

	int         hypervisor_width;
	int         image_width;
	int         kernel_width;
};

/** used by stdin and stdout socket watchers */
struct socket_watcher_data
{
	GMainLoop* loop;
	GIOChannel* socket_io;
	struct oci_state *state;
	gboolean setup_success;
};

/**
 * Used by watcher_runtime_dir(), handle_process_socket() and
 * handle_socket_close() to determine when the VM has finished starting
 * and also when it has shutdown.
 */
struct process_watcher_data
{
	struct cc_oci_config  *config;
	GMainLoop              *loop;
	GSocket                *socket;
	GSocketAddress         *src_address;
	GIOChannel             *channel;
	gboolean                failed;
};

/**
 * List of spec handlers used to process config on start
 */
static struct spec_handler* start_spec_handlers[] = {
	&annotations_spec_handler,
	&hooks_spec_handler,
	&mounts_spec_handler,
	&platform_spec_handler,
	&process_spec_handler,
	&root_spec_handler,
	&vm_spec_handler,
	&linux_spec_handler,

	/* terminator */
	NULL
};

/*!
 * Get the path of the specified file below the bundle path.
 *
 * \param bundle_path Full path to containers bundle path.
 * \param file Full path to file to find below \p bundle_path.
 *
 * \return Newly-allocated path string on success, else \c NULL.
 */
gchar *
cc_oci_get_bundlepath_file (const gchar *bundle_path,
		const gchar *file)
{
	if ((!bundle_path) || (!(*bundle_path)) ||
		(!file) || (!(*file))) {
		return NULL;
	}

	return g_build_path ("/", bundle_path, file, NULL);
}

/*!
 * Determine the containers config file, its configuration
 * and state.
 *
 * \param[out] config_file Dynamically-allocated path to containers
 * config file.
 * \param[out] config \ref cc_oci_config.
 * \param[out] state \ref oci_state.
 *
 * \note Used by the "stop" command.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_get_config_and_state (gchar **config_file,
		struct cc_oci_config *config,
		struct oci_state **state)
{
	if ((!config_file) || (!config) || (!state)) {
		return false;
	}

	if (! cc_oci_runtime_path_get (config)) {
			return false;
	}

	if (! cc_oci_state_file_get (config)) {
		return false;
	}

	*state = cc_oci_state_file_read (config->state.state_file_path);
	if (! (*state)) {
		g_critical("failed to read state file for container %s",
		           config->optarg_container_id);
		goto err;
	}

	/* Fill in further details to make the config valid */
	config->bundle_path = g_strdup ((*state)->bundle_path);
	config->state.workload_pid = (*state)->pid;
	config->state.status = (*state)->status;

	g_strlcpy (config->state.comms_path, (*state)->comms_path,
			sizeof (config->state.comms_path));

	g_strlcpy (config->state.procsock_path,
			(*state)->procsock_path,
			sizeof (config->state.procsock_path));

	*config_file = cc_oci_config_file_path ((*state)->bundle_path);
	if (! (*config_file)) {
		goto err;
	}

	return true;

err:
	cc_oci_state_free (*state);
	g_free_if_set (*config_file);
	return false;
}

/*!
 * Forcibly stop the Hypervisor.
 *
 * \param config \ref cc_oci_config.
 * \param state \ref oci_state.
 * \param signum Signal number to send to hypervisor.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_kill (struct cc_oci_config *config,
		struct oci_state *state,
		int signum)
{
	enum oci_status last_status;

	if (! (config && state)) {
		return false;
	}

	/* save current status */
	last_status = config->state.status;

	/* stopping container */
	config->state.status = OCI_STATUS_STOPPING;

	/* update state file */
	if (! cc_oci_state_file_create (config, state->create_time)) {
		g_critical ("failed to recreate state file");
		goto error;
	}

	if (kill (state->pid, signum) < 0) {
		g_critical ("failed to stop container %s "
				"running with pid %u: %s",
				config->optarg_container_id,
				(unsigned)state->pid,
				strerror (errno));
		/* revert container status */
		config->state.status = last_status;
		if (! cc_oci_state_file_create (config, state->create_time)) {
			g_critical ("failed to recreate state file");
		}
		return false;
	}

	last_status = config->state.status;

	config->state.status = OCI_STATUS_STOPPED;

	/* update state file */
	if (! cc_oci_state_file_create (config, state->create_time)) {
		g_critical ("failed to recreate state file");
		goto error;
	}

	return true;

error:
	/* revert container status */
	config->state.status = last_status;

	return false;
}

/*!
 * Determine if the VM is running.
 *
 * \param  state \ref oci_state.
 *
 * \return \c true on success, else \c false.
 */
private gboolean
cc_oci_vm_running (const struct oci_state *state)
{
	if (! (state && state->pid)) {
		return false;
	}

	return kill (state->pid, 0) == 0;
}

/*!
 * Watcher for socket stdin.
 *
 * \param  source GIOChannel.
 * \param  condition GIOCondition.
 * \param  buffer GString.
 *
 * \return \c false to ignore event.
 */
static gboolean
watcher_socket_stdin(GIOChannel* source, GIOCondition condition,
    GString* buffer)
{
	gsize bytes_written;
	GIOStatus status;

	if (condition == G_IO_HUP) {
		g_io_channel_unref(source);
		goto out;
	}
	do {
		status = g_io_channel_write_chars(source, buffer->str,
				(gssize)buffer->len, &bytes_written, NULL);
	}while(status == G_IO_STATUS_NORMAL && buffer->len != bytes_written );

	g_io_channel_flush(source, NULL);

out:
	g_string_free(buffer, true);

	/* unregister this watcher */
	return false;
}

/*!
 * Watcher for socket stdout.
 *
 * \param  source GIOChannel.
 * \param  condition GIOCondition.
 * \param  data \ref socket_watcher_data.
 *
 * \return \c false to ignore event.
 */
static gboolean
watcher_socket_stdout(GIOChannel* source, GIOCondition condition,
    struct socket_watcher_data *data)
{
	GIOStatus status;
	gchar buffer[LINE_MAX];
	gsize bytes_read;
	gboolean ret = true;

	if (condition == G_IO_HUP) {
		g_io_channel_unref(source);
		ret = false;
		goto out;
	}

	/* read and print all chars */
	while(true) {
		status = g_io_channel_read_chars(source, buffer, sizeof(buffer),
				&bytes_read, NULL);
		if (status != G_IO_STATUS_NORMAL) {
			break;
		}
		if (bytes_read < sizeof(buffer)) {
			g_strlcpy(buffer+bytes_read, "", 1);
		}
		g_print("%s", buffer);
	}
out:
	/* if vm is not running exit */
	if (! cc_oci_vm_running(data->state)) {
		if (data->loop) {
			g_main_loop_quit(data->loop);
			return false;
		}
	}

	return ret;
}

/*!
 * Watcher for stdin.
 *
 * \param  source GIOChannel.
 * \param  condition GIOCondition.
 * \param  data \ref socket_watcher_data.
 *
 * \return \c false to ignore event.
 */
static gboolean
watcher_stdin(GIOChannel* source, GIOCondition condition,
    struct socket_watcher_data *data)
{
	GIOStatus status;
	GString* buffer;
	gboolean ret = true;

	if (condition == G_IO_HUP) {
		g_io_channel_unref(source);
		ret = false;
		goto out;
	}

	buffer = g_string_new("");
	status = g_io_channel_read_line_string(source, buffer, NULL, NULL);
	if (status != G_IO_STATUS_NORMAL) {
		if (buffer) {
			g_string_free(buffer, true);
		}
		goto out;
	}

	/* buffer will be freed by watcher_socket_stdin */
	g_io_add_watch(data->socket_io, G_IO_OUT | G_IO_HUP,
		(GIOFunc)watcher_socket_stdin, buffer);
out:
	/* if vm is not running exit */
	if (! cc_oci_vm_running(data->state)) {
		if (data->loop) {
			g_main_loop_quit(data->loop);
			return false;
		}
	}

	return ret;
}

/*!
 * Attach to the Hypervisor.
 *
 * \param config \ref cc_oci_config.
 * \param state \ref oci_state.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_attach (struct cc_oci_config *config,
		struct oci_state *state)
{
	gboolean result = false;
	GSocket* socket;
	GSocketAddress* src_address;
	GError *error = NULL;
	int socket_fd;
	GIOChannel* socket_io;
	GIOChannel* std_in;
	GMainLoop* loop;
	struct socket_watcher_data data = { 0 };

	loop = g_main_loop_new (NULL, false);
	if (! loop) {
		g_critical("failed to create main loop");
		return false;
	}

	socket = g_socket_new(G_SOCKET_FAMILY_UNIX, G_SOCKET_TYPE_STREAM, 0, &error);
	if (! socket) {
		g_critical("failed to create unix socket");
		if (error) {
			g_critical("error: %s", error->message);
			g_error_free (error);
		}
		goto fail1;
	}

	src_address = g_unix_socket_address_new_with_type(state->console, -1,
			G_UNIX_SOCKET_ADDRESS_PATH);
	if (! src_address) {
		g_critical("failed to create socket address: %s", state->console);
		goto fail2;
	}

	if (! g_socket_connect(socket, src_address, NULL, &error)) {
		g_critical("failed to connect socket");
		if (error) {
			g_critical("error: %s", error->message);
			g_error_free (error);
		}
		goto fail3;
	}

	/* ensure non-blocking mode */
	g_socket_set_blocking(socket, false);

	/* get socket fd to create GIOChannel */
	socket_fd = g_socket_get_fd(socket);

	/* add watcher to socket stdout */
	socket_io = g_io_channel_unix_new(socket_fd);
	if (! socket_io) {
		g_critical("failed to create io channel");
		goto fail3;
	}

	std_in = g_io_channel_unix_new(STDIN_FILENO);
	if (! std_in) {
		g_critical("failed to create io channel");
		goto fail4;
	}

	data.loop = loop;
	data.state = state;
	data.socket_io = socket_io;

	/* add stdin watcher */
	g_io_add_watch(std_in, G_IO_IN | G_IO_HUP,
	    (GIOFunc)watcher_stdin, &data);

	/* add socket stdout watcher */
	g_io_add_watch(socket_io, G_IO_IN | G_IO_HUP,
	    (GIOFunc)watcher_socket_stdout, &data);

	data.setup_success = true;

	/* run main loop */
	g_main_loop_run(loop);

	result = true;

	/* Free memory.
	 *
	 * Note that watcher_* functions handle freeing the channels
	 * normally.
	 */
fail4:
	g_io_channel_shutdown(socket_io, true, NULL);
	if (! data.setup_success) {
		 g_io_channel_unref (socket_io);
	}
fail3:
	g_object_unref(src_address);
fail2:
	g_object_unref(socket);
fail1:
	g_main_loop_unref(loop);

	return result;
}

/*!
 * Get the home directory for the workload user
 *
 * \param config \ref cc_oci_config.
 * \param passwd_path Path to the local passwd file
 *
 * \return Newly-allocated path string on success, else \c NULL.
 */

private gchar*
get_user_home_dir(struct cc_oci_config *config, gchar *passwd_path) {
	gchar          *user_home = NULL;
	FILE           *pw_file = NULL;
	struct passwd  *pw_entry;

	if (! (config && passwd_path)) {
		return NULL;
	}

	pw_file = g_fopen (passwd_path, "r");
	if ( pw_file == NULL) {
		g_warning("Could not open password file: %s\n", passwd_path);
		return NULL;
	}

	while ((pw_entry = fgetpwent(pw_file)) != NULL) {
		if (pw_entry->pw_uid == config->oci.process.user.uid) {
			user_home = g_strdup(pw_entry->pw_dir);
			break;
		}
	}

	fclose(pw_file);
	return user_home;
}

/*!
 * Set the HOME environment variable
 *
 * \param config \ref cc_oci_config.
 *
 * returns early if HOME is present in the environment configuration in \p config
 */
private void
set_env_home(struct cc_oci_config *config)
{
	g_autofree gchar *user_home_dir = NULL;
	g_autofree gchar *passwd_path = NULL;

	if (! (config && config->oci.process.env)) {
		return;
	}

	/* Check if HOME is set in the environment config */
	for (gchar **var = config->oci.process.env; *var != NULL; var++) {
		if (g_str_has_prefix (*var, "HOME=")) {
			g_debug("Home is already set in the configuration\n");
			return;
		}
	}

	guint env_len = 1 + g_strv_length(config->oci.process.env);
	gchar **new_env = g_new0(gchar*, env_len + 1);

	passwd_path = g_strdup_printf ("%s/%s", config->oci.root.path, PASSWD_PATH);
	user_home_dir = get_user_home_dir(config, passwd_path);

	if (! user_home_dir) {
		// Fallback to stateless path 
		g_free(passwd_path);
		passwd_path = g_strdup_printf ("%s/%s", config->oci.root.path,
						STATELESS_PASSWD_PATH);
		user_home_dir = get_user_home_dir(config, passwd_path);

		// If we are not able to retrieve the home dir, set the default as "/"
		if (! user_home_dir) {
			user_home_dir = g_strdup("/");
			g_debug("No HOME found in environment, so setting HOME %s for user %d",
				user_home_dir, config->oci.process.user.uid);
		}
	}
	new_env[0] = g_strdup_printf("HOME=%s", user_home_dir);

	for (int i = 0; i < env_len-1; i++) {
		new_env[i+1] = g_strdup(config->oci.process.env[i]);
	}

	g_strfreev(config->oci.process.env);
	config->oci.process.env = new_env;
}


/*!
 * Create the containers Clear Linux workload file
 * (\ref CC_OCI_WORKLOAD_FILE).
 *
 * \param config \ref cc_oci_config.
 *
 * \warning FIXME: Need to support running the workload as a different user/group.
 * Something like:
 *
 *      su -c "sg $group -c $cmd" $user
 *
 * \return \c true on success, else \c false.
 */
private gboolean
cc_oci_create_container_workload (struct cc_oci_config *config)
{
	GString           *contents = NULL;
	GError            *err = NULL;
	gchar             *workload_cmdline = NULL;
	g_autofree gchar  *path = NULL;
	g_autofree gchar   *cwd = NULL;
	gboolean           ret = false;
	gchar             **args = NULL;

	if (! (config && config->oci.process.args)) {
		return false;
	}

	if (! config->vm) {
		g_critical ("No vm configuration");
		goto out;
	}

	if (config->oci.process.env) {
		g_autofree gchar  *envpath = NULL;
		g_autofree gchar  *env = NULL;

		envpath = g_build_path ("/", config->oci.root.path,
				CC_OCI_ENV_FILE, NULL);
		if (! envpath) {
			return false;
		}

		set_env_home(config);

		env = g_strjoinv ("\n", config->oci.process.env);
		ret = g_file_set_contents (envpath, env, -1, &err);
		if (! ret) {
			g_critical ("failed to create environment file (%s): %s",
					envpath, err->message);
			g_error_free (err);
			goto out;
		}
	}

	path = g_build_path ("/", config->oci.root.path,
			CC_OCI_WORKLOAD_FILE, NULL);
	if (! path) {
		return false;
	}

	cwd = g_shell_quote (config->oci.process.cwd);
	if (! cwd) {
		return false;
	}

	g_strlcpy (config->vm->workload_path, path,
			sizeof (config->vm->workload_path));

	args = config->oci.process.args;
	while (*args != NULL) {
		gchar *new_arg = g_shell_quote(*args);
		g_free(*args);
		*args = new_arg;
		args++;
	}

	workload_cmdline = g_strjoinv (" ", config->oci.process.args);
	if (! workload_cmdline) {
		goto out;
	}

	contents = g_string_new("");
	if (! contents) {
		goto out;
	}

	/* TODO: we should probably force failure if the 'cd' fails */
	g_string_printf(contents,
			"#!%s\n"
			"cd %s\n"
			"%s\n",
			CC_OCI_WORKLOAD_SHELL,
			cwd,
			workload_cmdline);

	ret = g_file_set_contents (config->vm->workload_path,
			contents->str, (gssize)contents->len, &err);

	if (! ret) {
		g_critical ("failed to create workload file (%s): %s",
				config->vm->workload_path, err->message);
		g_error_free (err);
		goto out;
	}

	if (g_chmod (config->vm->workload_path, CC_OCI_SCRIPT_MODE) < 0) {
		g_critical ("failed to set mode for file file %s",
				config->vm->workload_path);
		goto out;
	}

	g_debug ("created workload_path %s", config->vm->workload_path);

	ret = true;

out:
	g_free_if_set (workload_cmdline);
	if (contents) {
		g_string_free(contents, true);
	}

	return ret;
}

/*!
 * Clean up all resources (including unmounts) for
 * the specified config.
 *
 * \param config \ref cc_oci_config.
 *
 * \return \c true on success, else \c false.
 */
static gboolean
cc_oci_cleanup (struct cc_oci_config *config)
{
	g_assert (config);

	if (! cc_oci_handle_unmounts (config)) {
		return false;
	}

	if (! cc_oci_state_file_delete (config)) {
		return false;
	}

	if (! cc_oci_runtime_dir_delete (config)) {
		return false;
	}

	return true;
}

/*!
 * Parse the \c GNode representation of \ref CC_OCI_CONFIG_FILE
 * and save values in the provided \ref cc_oci_config.
 *
 * \param config \ref cc_oci_config.
 *
 * \return \c true on success, else \c false.
 */
static gboolean
cc_oci_config_file_parse (struct cc_oci_config *config)
{
	g_autofree gchar  *config_file = NULL;
	g_autofree gchar  *cwd = NULL;
	GNode             *root = NULL;
	gboolean           ret = false;

	if (! config || ! config->bundle_path) {
		return false;
	}

	config_file = cc_oci_config_file_path (config->bundle_path);
	if (! config_file) {
		return false;
	}

	g_debug ("using config_file %s", config_file);

	cwd = g_get_current_dir ();
	if (! cwd) {
		return false;
	}

	/* Set bundle directory as working directory. This is required
	 * to deal with relative paths (paths relative to the bundle
	 * directory) in CC_OCI_CONFIG_FILE which must
	 * be resolved to absolutes.
	 */
	if (g_chdir (config->bundle_path) != 0) {
		g_critical ("Cannot chdir to %s: %s",
				config->bundle_path,
				strerror (errno));
		return false;
	}

	/* convert json file to GNode */
	if (! cc_oci_json_parse (&root, config_file)) {
		goto out;
	}

#ifdef DEBUG
	/* show json file converted to GNode */
	cc_oci_node_dump (root);
#endif /*DEBUG*/

	/* parse the GNode representation of CC_OCI_CONFIG_FILE */
	if (! cc_oci_process_config(root, config, start_spec_handlers)) {
		g_critical ("failed to process config");
		goto out;
	}

	/* Supplement the OCI config by determining VM configuration
	 * details.
	 */
	if (! get_spec_vm_from_cfg_file (config)) {
		g_critical ("failed to find any sources of VM configuration");
		goto out;
	}

	ret = true;

out:
	g_free_node (root);

	(void)g_chdir (cwd);

	return ret;
}

/*!
 * Create the state file, apply mounts and run hooks,
 * but do not start the VM.
 *
 * \param config \ref cc_oci_config.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_create (struct cc_oci_config *config)
{
	gboolean  ret = false;

	if (! config) {
		return false;
	}

	if (! cc_oci_config_file_parse (config)) {
		return false;
	}

	if (! cc_oci_config_check (config)) {
		return false;
	}

	if (! cc_oci_runtime_dir_setup (config)) {
		if (g_file_test (config->state.runtime_path,
					G_FILE_TEST_EXISTS |
					G_FILE_TEST_IS_DIR)) {
			g_critical ("container %s already exists",
					config->optarg_container_id);
		} else {
			g_critical ("failed to create runtime directory");
		}

		return false;
	}

	if (! cc_oci_handle_mounts (config)) {
		g_critical ("failed to handle mounts");
		return false;
	}

	if (! cc_oci_create_container_workload (config)) {
		g_critical ("failed to create workload");
		return false;
	}

	// FIXME: consider dry-run mode.
	if (config->dry_run_mode) {
		g_debug ("dry-run mode: not launching VM");
		return true;
	}

	/* start VM is a stopped state (containerd requires a
	 * valid pid in the pidfile after a successful "create").
	 */
	if (! cc_oci_vm_launch (config)) {
		g_critical ("failed to launch VM");
		goto out;
	}

	ret = true;

out:
	return ret;
}

/*!
 * Called when the \ref CC_OCI_PROCESS_SOCKET file is closed by the VM,
 * denoting process shutdown.
 *
 * \param source \c GIOChannel.
 * \param condition \c GIOCondition.
 * \param data \ref process_watcher_data.
 *
 * \return \c false on success, else \c true.
 */
static gboolean
handle_socket_close (GIOChannel              *source,
		GIOCondition                  condition,
		struct process_watcher_data  *data)
{
	(void)source;
	(void)condition;

	g_assert (data);
	g_assert (data->loop);

	g_main_loop_quit (data->loop);

	/* signify success */
	return false;
}

/**
 * Connect to \ref CC_OCI_PROCESS_SOCKET and set a watch to trigger
 * when the socket is closed (denoting the VM has shutdown).
 *
 * \param data \ref process_watcher_data.
 *
 * \return \c true on success, else \c false.
 */
static gboolean
handle_process_socket (struct process_watcher_data *data)
{
	gboolean         ret = false;
	int              socket_fd;
	GError          *error = NULL;
	const gchar     *path;

	if (! data || ! data->loop || ! data->config) {
		return false;
	}

	if (! data->config->state.procsock_path[0]) {
		return false;
	}

	path = data->config->state.procsock_path;

	data->socket = g_socket_new (G_SOCKET_FAMILY_UNIX,
			G_SOCKET_TYPE_STREAM, 0, &error);
	if (! data->socket) {
		g_critical("failed to create unix socket: %s",
				error->message);
		g_error_free (error);
		goto fail1;
	}

	data->src_address = g_unix_socket_address_new_with_type (path,
			-1,
			G_UNIX_SOCKET_ADDRESS_PATH);
	if (! data->src_address) {
		g_critical("failed to create socket address: %s",
				data->config->state.procsock_path);
		goto fail2;
	}

	ret = g_socket_connect(data->socket, data->src_address,
			NULL, &error);
	if (! ret) {
		g_critical("failed to connect to socket: %s",
				error->message);
		g_error_free (error);
		goto fail3;
	}

	/* ensure non-blocking mode */
	g_socket_set_blocking (data->socket, false);

	/* get socket fd to create GIOChannel */
	socket_fd = g_socket_get_fd (data->socket);

	data->channel = g_io_channel_unix_new (socket_fd);
	if (! data->channel) {
		g_critical ("failed to create io channel");
		goto fail4;
	}

	g_io_add_watch (data->channel,
			G_IO_HUP|G_IO_ERR,
			(GIOFunc)handle_socket_close,
			data);

	return true;

fail4:
	g_io_channel_shutdown (data->channel, true, NULL);
	g_io_channel_unref (data->channel);
fail3:
	g_object_unref (data->src_address);
fail2:
	g_object_unref (data->socket);
fail1:
	g_main_loop_unref (data->loop);
	data->loop = NULL;

	return false;
}

/**
 * Determine when \ref CC_OCI_PROCESS_SOCKET is created.
 *
 * \param monitor \c GFileMonitor.
 * \param file \c GFile.
 * \param other_file \c GFile (unused).
 * \param event_type \c GFileMonitorEvent.
 * \param data \ref process_watcher_data.
 */
static void
watcher_runtime_dir (GFileMonitor            *monitor,
		GFile                        *file,
		GFile                        *other_file,
		GFileMonitorEvent             event_type,
		struct process_watcher_data  *data)
{
	g_autofree gchar  *path = NULL;
	g_autofree gchar  *name = NULL;

	(void)other_file;

	g_assert (data);

	if (event_type != G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT) {
		return;
	}

	path = g_file_get_path (file);
	if (! path) {
		return;
	}

	name = g_path_get_basename (path);

	/* ignore non-matching files */
	if (g_strcmp0 (CC_OCI_PROCESS_SOCKET, name)) {
		return;
	}

	/* CC_OCI_PROCESS_SOCKET has now been created, so delete the
	 * monitor.
	 */
	g_object_unref (monitor);

	/* Now that the socket has been created, connect to it */
	if (! handle_process_socket (data)) {
		data->failed = true;
		g_critical ("failed to handle process socket");
	}
}

/*!
 * Start a VM previously setup by a call to cc_oci_create().
 *
 * \param config \ref cc_oci_config.
 * \param state \ref oci_state.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_start (struct cc_oci_config *config,
		struct oci_state *state)
{
	gboolean       ret = false;
	GPid           pid;
	GFileMonitor  *monitor = NULL;
	GFile         *file = NULL;
	GError        *error = NULL;
	gboolean       wait = false;
	struct process_watcher_data data = { 0 };
	gchar         *config_file = NULL;

	if (! config || ! state) {
		return false;
	}

	if (state->status == OCI_STATUS_RUNNING) {
		if (cc_oci_vm_running (state)) {
			g_critical ("container %s is already running",
					config->optarg_container_id);
		} else {
			/* pid from state file is not / no longer valid */
			g_critical ("container no longer running");
		}

		return false;

	} else if (state->status != OCI_STATUS_CREATED) {
		g_critical ("unexpected state for container %s: %s",
				config->optarg_container_id,
				cc_oci_status_to_str (state->status));
		return false;
	}

	/* FIXME: how can we handle a "start --bundle=..." override? */
	if (start_data.bundle) {
		if (config->bundle_path) {
			g_free (config->bundle_path);
		}

		config->bundle_path = cc_oci_resolve_path (start_data.bundle);
		g_free (start_data.bundle);
		start_data.bundle = NULL;
	}

	pid = config->state.workload_pid;

	/* XXX: If running stand-alone, wait for the hypervisor to
	 * finish. But if running under containerd, don't wait.
	 *
	 * A simple way to determine if we're being called
	 * under containerd is to check if stdin is closed.
	 *
	 * Do not wait when console is empty.
	 */
	if ((config->oci.process.terminal && ! config->detached_mode) &&
	    !config->use_socket_console) {
		wait = true;
	}

	if (wait) {
		data.config = config;
		data.loop = g_main_loop_new (NULL, 0);
		if (! data.loop) {
			g_critical ("cannot create main loop for client");
			return false;
		}

		file = g_file_new_for_path (config->state.runtime_path);
		if (! file) {
			g_main_loop_unref (data.loop);
			return false;
		}

		/* create inotify watch on runtime directory
		 * (crucially before the VM is resumed).
		 */
		monitor = g_file_monitor_directory (file,
				G_FILE_MONITOR_WATCH_MOVES,
				NULL, &error);
		if (! monitor) {
			g_critical ("failed to monitor %s: %s",
					g_file_get_path (file),
					error->message);
			g_error_free (error);
			g_object_unref (file);
			g_main_loop_unref (data.loop);

			return false;
		}

		g_signal_connect (monitor, "changed",
				G_CALLBACK (watcher_runtime_dir),
				&data);
	}

	/* "create" left the VM in a stopped state, so now let it
	 * continue.
	 *
	 * This will result in \ref CC_OCI_PROCESS_SOCKET being
	 * created, however there will be a delay. Since we wish to
	 * connect to this socket, the approach is to use an inotify
	 * watch to wait for the socket file to exist, then connect to
	 * it.
	 */
	if (kill (pid, SIGCONT) < 0) {
		g_critical ("failed to start VM %s: %s",
				config->optarg_container_id,
				strerror (errno));
		return false;
	}

	g_debug ("activated VM %s (pid %d)",
			config->optarg_container_id,
			(int)pid);

	/* Now the VM is running */
	config->state.status = OCI_STATUS_RUNNING;

	/* update state file after run container */
	if (! cc_oci_state_file_create (config, state->create_time)) {
		g_critical ("failed to recreate state file");
		ret = false;
		goto out;
	}

	/* If a hook returns a non-zero exit code, then an error is
	logged and the remaining hooks are executed. */
	cc_run_hooks (config->oci.hooks.poststart,
	              config->state.state_file_path, false);

	if (wait) {
		g_main_loop_run (data.loop);

		/* Read state file to detect if the VM was stopped */
		ret = cc_oci_get_config_and_state (&config_file, config,
				&state);
		if (! ret) {
			goto out;
		}

		/* If the VM was stopped then *do not* cleanup */
		if (config->state.status != OCI_STATUS_STOPPED &&
			config->state.status != OCI_STATUS_STOPPING) {
			ret = cc_oci_cleanup (config);
			if (data.failed) {
				ret = false;
			}
		}
	} else {
		ret = true;
	}

out:
	if (wait) {
		if (file) {
			g_object_unref (file);
		}
		if (data.channel) {
			g_io_channel_shutdown (data.channel, true, NULL);
			g_io_channel_unref (data.channel);
		}
		if (data.src_address) {
			g_object_unref (data.src_address);
		}
		if (data.socket) {
			g_object_unref (data.socket);
		}
		if (data.loop) {
			g_main_loop_unref (data.loop);
			data.loop = NULL;
		}
		g_free_if_set (config_file);
	}

	return ret;
}

/*!
 * Start the hypervisor and run the workload.
 *
 * \param config \ref cc_oci_config.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_run (struct cc_oci_config *config)
{
	struct oci_state  *state;

	if (! config) {
		return false;
	}

	if (! cc_oci_create (config)) {
		return false;
	}

	/* FIXME: Inefficient - cc_oci_create() has already created the
	 * state file, so should not need to re-read it!!
	 *
	 * we could potentially associate config and state
	 * (config->state), but great care would need to be taken on
	 * cleanup.
	 */
	state = cc_oci_state_file_read (config->state.state_file_path);
	if (! state) {
		g_critical ("failed to read state file "
				"for container %s",
				config->optarg_container_id);
		return false;
	}

	/* Update the config object based on the state.
	 *
	 * This is required since the child process that becomes the
	 * hypervisor has now updated the on-disk state file. But the
	 * parents state object does not reflect the on-disk state.
	 */
	if (! cc_oci_config_update (config, state)) {
		return false;
	}

	if (! cc_oci_start (config, state)) {
		return false;
	}

	return true;
}

/*!
 * Stop the Hypervisor.
 *
 * \param config \ref cc_oci_config.
 * \param state \ref oci_state.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_stop (struct cc_oci_config *config,
		struct oci_state *state)
{
	gboolean  ret;

	g_assert (config);
	g_assert (state);

	if (cc_oci_vm_running (state)) {
		ret = cc_oci_vm_shutdown (state->comms_path, state->pid);
		if (! ret) {
			return false;
		}
	} else {
		/* This isn't a fatal condition since:
		 *
		 * - containerd calls "delete" twice (unclear why).
		 * - Even if the VM has already shutdown, it's still
		 *   necessary to perform cleanup (unmounting, etc).
		 */
		g_warning ("Cannot delete VM %s (pid %u) - "
				"not running",
				state->id, state->pid);
	}

	ret = cc_oci_cleanup (config);

	/* The post-stop hooks are called after the container process is
	 * stopped. Cleanup or debugging could be performed in such a
	 * hook. If a hook returns a non-zero exit code, then an error
	 * is logged and the remaining hooks are executed.
	 */
	cc_run_hooks (config->oci.hooks.poststop,
	              config->state.state_file_path, false);

	return ret;
}

/*!
 * Toggle the state of the Hypervisor.
 *
 * \param config \ref cc_oci_config.
 * \param state \ref oci_state.
 * \param pause If \c true, pause the VM, else resume it.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_toggle (struct cc_oci_config *config,
		struct oci_state *state,
		gboolean pause)
{
	gboolean        (*fp) (const gchar *socket_path, GPid pid);
	enum oci_status   dest_status;
	gboolean          ret;

	g_assert (config);
	g_assert (state);

	dest_status = pause ? OCI_STATUS_PAUSED : OCI_STATUS_RUNNING;

	if (state->status == dest_status) {
		g_warning ("already %s",
				cc_oci_status_to_str (state->status));
		return true;
	}

	fp = pause ? cc_oci_vm_pause : cc_oci_vm_resume;

	ret = fp (state->comms_path, state->pid);
	if (! ret) {
		return false;
	}

	config->state.status = dest_status;

	return cc_oci_state_file_create (config, state->create_time);
}

/*!
 * Run the command specified by \p argv in the hypervisor
 * and wait for it to finish.
 *
 * \param config \ref cc_oci_config.
 * \param state \ref oci_state.
 * \param argc Argument count.
 * \param argv Argument vector.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_exec (struct cc_oci_config *config,
		struct oci_state *state,
		int argc,
		char *const argv[])
{
	g_assert (config);
	g_assert (state);
	g_assert (argc);
	g_assert (argv);

	if (! cc_oci_vm_connect (config, argc, argv)) {
		g_critical ("failed to connect to VM");
		return false;
	}

	return true;
}

/*!
 * Display details of a VM.
 *
 * \param state State of VM (\ref oci_state).
 * \param options Options for how to display the VM details
 * (\ref format_options).
 *
 * \note FIXME: maybe we should simply not display a VM if it is destroyed?
 */
static void
cc_oci_list_vm (const struct oci_state *state,
		const struct format_options *options)
{
	JsonObject  *obj = NULL;
	const gchar  *status = NULL;

	g_assert (state);
	g_assert (options);

	if (! cc_oci_vm_running (state)) {
		status = cc_oci_status_to_str (OCI_STATUS_STOPPED);
	} else {
		status = cc_oci_status_to_str (state->status);
	}

	if (! options->use_json) {
		g_print ("%-*s ", options->id_width, state->id);

		/* XXX: It doesn't seem to be possible to display an
		 * unsigned value using a minimum field width *iff* the
		 * value is zero.
		 *
		 * We need to be able to display zero to represent an
		 * unstarted container, hence this unsavoury test.
		 */
		if (! state->pid) {
			g_print ("%-*.*s ",
					options->pid_width,
					options->pid_width,
					"0");
		} else {
			g_print ("%-*.u ",
					options->pid_width,
					(unsigned)state->pid);
		}

		g_print ("%-*s %-*s %-*s%s",
				options->status_width,
				status,

				options->bundle_width,
				state->bundle_path,

				options->created_width,
				state->create_time,

				options->show_all ? " " : "\n");

		if (options->show_all) {
			g_print ("%-*s %-*s %-*s\n",
					options->hypervisor_width,
					state->vm->hypervisor_path,

					options->kernel_width,
					state->vm->kernel_path,

					options->image_width,
					state->vm->image_path);
		}

		return;
	}

	obj = json_object_new ();

	json_object_set_string_member (obj, "id", state->id);
	json_object_set_int_member (obj, "pid", state->pid);

	json_object_set_string_member (obj, "status", status);

	json_object_set_string_member (obj, "bundle", state->bundle_path);
	json_object_set_string_member (obj, "created", state->create_time);

	if (options->show_all) {
		json_object_set_string_member (obj, "hypervisor",
				state->vm->hypervisor_path);

		json_object_set_string_member (obj, "kernel",
				state->vm->kernel_path);

		json_object_set_string_member (obj, "image",
				state->vm->image_path);
	}

	/* The array now owns the object, so no need to free it */
	json_array_add_object_element (options->array, obj);
}

/*!
 * Get the state of a VM.
 *
 * \param name Name of VM.
 * \param root_dir Root directory to use for runtime.
 *
 * Note that error checking has to be lax here since:
 *
 * - the VM may be destroyed as this function runs.
 *
 * \return \ref oci_state on success, else \c NULL.
 */
static struct oci_state *
cc_oci_vm_get_state (const gchar *name, const char *root_dir)
{
	struct cc_oci_config  config = { { 0 } };
	struct oci_state      *state = NULL;
	gchar                 *config_file = NULL;

	g_assert (name);

	config.optarg_container_id = name;

	if (root_dir) {
		config.root_dir = g_strdup (root_dir);
	}

	if (! cc_oci_runtime_path_get (&config)) {
		return NULL;
	}

	if (! cc_oci_state_file_get (&config)) {
		return NULL;
	}

	state = cc_oci_state_file_read (config.state.state_file_path);

	g_free_if_set (config_file);
	cc_oci_config_free (&config);

	return state;
}

/*!
 * Update the widths required to display a VM.
 *
 * \param state State of VM (\ref oci_state).
 * \param options Options for how to display the VM details
 * (\ref format_options).
 *
 * \return \ref oci_state on success, else \c NULL.
 *
 * \todo FIXME: This function needs to consider not only the width of
 * the values, but also the width of the column headings (see the extra
 * test required to handle PIDs in the code below).
 */
static void
cc_oci_update_options (const struct oci_state *state,
		struct format_options *options)
{
	static int   status_max = 0;
	GString     *str = g_string_new("");

	g_assert (state);
	g_assert (state->vm);
	g_assert (options);

	if (! status_max) {
		status_max = cc_oci_status_length ();
		options->status_width = status_max;
	}

	g_string_assign(str, state->id);
	options->id_width = CC_OCI_MAX (options->id_width,
			(int)str->len);

	g_string_printf(str, "%u", (unsigned)state->pid);
	options->pid_width = CC_OCI_MAX (options->pid_width,
			(int)str->len);

	/* XXX: a PID may be shorter than its column heading, so handle
	 * that.
	 */
	options->pid_width = CC_OCI_MAX (options->pid_width,
			(int)sizeof("PID")-1);

	g_string_assign(str, state->bundle_path);
	options->bundle_width = CC_OCI_MAX (options->bundle_width,
			(int)str->len);

	g_string_assign(str, state->create_time);
	options->created_width = CC_OCI_MAX (options->created_width,
			(int)str->len);

	g_string_assign(str, state->vm->hypervisor_path);
	options->hypervisor_width = CC_OCI_MAX (options->hypervisor_width,
			(int)str->len);

	g_string_assign(str, state->vm->image_path);
	options->image_width = CC_OCI_MAX (options->image_width,
			(int)str->len);

	g_string_assign(str, state->vm->kernel_path);
	options->kernel_width = CC_OCI_MAX (options->kernel_width,
			(int)str->len);

	g_string_free(str, true);
}

/*!
 * List all VMs.
 *
 * Note that error checking has to be lax here since:
 *
 * - There may be no VMS to report on.
 * - VMs may be destroyed as this function runs.
 *
 * \param config \ref cc_oci_config.
 * \param format Type of format to present list in ("json", "table",
 * or NULL for text).
 * \param show_all If \c true, show all details.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_list (struct cc_oci_config *config, const gchar *format,
        gboolean show_all)
{
	GDir                   *dir;
	const gchar            *dirname;
	const gchar            *name;
	GSList                 *vms = NULL;
	struct oci_state       *state = NULL;
	gchar                  *str = NULL;
	struct format_options   options = { 0 };

	if ((!config) || (!format) || (!(*format))) {
		return false;
	}

	dirname = config->root_dir
		? config->root_dir
		: CC_OCI_RUNTIME_DIR_PREFIX;

	if (! g_strcmp0 (format, "json")) {
		options.use_json = true;
	} else if (! g_strcmp0 (format, "table")) {
		; /* NOP */
	} else {
		g_critical ("invalid list format: %s", format);
		return false;
	}

	options.show_all = show_all;

	dir = g_dir_open (dirname, 0x0, NULL);
	if (! dir) {
		/* No containers yet, so not an error */
		goto no_vms;
	}

	/* Read all VM state files and add to a list */
	while ((name = g_dir_read_name (dir)) != NULL) {
		gboolean ret;
		gchar *path;

		path = g_build_path ("/", dirname, name, NULL);

		ret = g_file_test (path, G_FILE_TEST_IS_DIR);
		if (! ret) {
			g_free (path);
			continue;
		}

		g_free (path);

		state = cc_oci_vm_get_state (name, dirname);
		if (! state) {
			continue;
		}

		if (! options.use_json) {
			/* calculate the maximum field widths
			 * to display the state values.
			 */
			cc_oci_update_options (state, &options);
		}

		vms = g_slist_append (vms, state);
	}

no_vms:
	if (options.use_json) {
		if (! vms) {
			/* List is empty */
			/* Be runc compatible */
			g_print ("%s", "null");

			goto out;
		} else {
			options.array = json_array_new ();
		}
	} else {
		/* format the header using the calculated widths */
		g_print ("%-*s %-*s %-*s %-*s %-*s%s",
				options.id_width,
				"ID",

				options.pid_width,
				"PID",

				options.status_width,
				"STATUS",

				options.bundle_width,
				"BUNDLE",

				options.created_width,
				"CREATED",

				options.show_all ? " " : "\n");

		if (options.show_all) {
			g_print ("%-*s %-*s %-*s\n",
					options.hypervisor_width,
					"HYPERVISOR",

					options.kernel_width,
					"KERNEL",

					options.image_width,
					"IMAGE");
		}
	}

	/* display the VMs, again using the calculated widths */
	g_slist_foreach (vms, (GFunc)cc_oci_list_vm, &options);

	if (options.use_json) {
		str = cc_oci_json_arr_to_string (options.array, false);
		if (! str) {
			goto out;
		}

		g_print ("%s\n", str);
		json_array_unref (options.array);
	}

	/* clean up */
	g_slist_free_full (vms, (GDestroyNotify)cc_oci_state_free);

out:
	if (dir) {
		g_dir_close (dir);
	}
	g_free_if_set (str);

	return true;
}

/**
 * Transfer certain elements from \p state to \p config.
 *
 * This is required since a state file is only ever generated from a
 * \ref cc_oci_config object.
 *
 * \param config \ref cc_oci_config.
 * \param state \ref oci_state.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_config_update (struct cc_oci_config *config,
		struct oci_state *state)
{
	if (! (config && state)) {
		return false;
	}

	if (state->mounts) {
		config->oci.mounts = state->mounts;
		state->mounts = NULL;
	}

	if (state->console) {
		config->console = state->console;
		state->console = NULL;
	}

	config->use_socket_console = state->use_socket_console;

	if (config->console && ! config->use_socket_console && isatty (STDIN_FILENO)) {
		g_debug ("enabling terminal for standalone mode");
		config->oci.process.terminal = true;
	}

	if (state->vm) {
		config->vm = state->vm;
		state->vm = NULL;
	}

	if (state->procsock_path) {
		/* No need to do a full transfer */
		g_strlcpy (config->state.procsock_path,
				state->procsock_path,
				sizeof (config->state.procsock_path));
	}

	return true;
}
