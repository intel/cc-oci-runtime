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

/*
 * Architecture:
 *
 * - fork.
 * - set std streams to the console device specified by containerd.
 * - exec the hypervisor.
 * - parent exits. This causes the child (hypervisor) to be reparented
 *   to its containerd-shim process, since that has been configured as a
 *   sub-reaper.
 */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include <gio/gunixsocketaddress.h>
#include <gio/gunixconnection.h>

#include "oci.h"
#include "util.h"
#include "hypervisor.h"
#include "process.h"
#include "state.h"
#include "namespace.h"
#include "networking.h"
#include "common.h"
#include "logging.h"
#include "netlink.h"
#include "proxy.h"

static GMainLoop* main_loop = NULL;
private GMainLoop* hook_loop = NULL;

/** List of shells that are recognised by "exec", ordered by likelihood. */
static const gchar *recognised_shells[] =
{
	"sh",
	"bash",
	"zsh",
	"ksh",
	"csh",

	/* terminator */
	NULL,
};

/*!
 * Determine if \p cmd is a shell.
 *
 * \param cmd Command to check.
 *
 * \return \c true on success, else \c false.
 */
private gboolean
cc_oci_cmd_is_shell (const char *cmd)
{
	const gchar **shell;

	if (! cmd) {
		return false;
	}

	for (shell = (const gchar **)recognised_shells;
			shell && *shell;
			shell++) {
		g_autofree gchar *suffix = NULL;

		if (! g_strcmp0 (*shell, cmd)) {
			/* exact match */
			return true;
		}

		suffix = g_strdup_printf ("/%s", *shell);

		if (g_str_has_suffix (cmd, suffix)) {
			/* full path to shell was specified */
			return true;
		}
	}

	return false;
}

/*!
 * Close file descriptors, excluding standard streams.
 *
 * \param fds Array of file descriptors that should not be closed
 *
 * \return \c true on success, else \c false.
 */
static gboolean
cc_oci_close_fds (GArray *fds) {
	char           *fd_dir = "/proc/self/fd";
	DIR            *dir;
	struct dirent  *ent;
	int             fd;

	dir = opendir (fd_dir);

	if (! dir) {
		return false;
	}

	while ((ent = readdir (dir)) != NULL) {
		if (! (g_strcmp0 (ent->d_name, ".") &&
		    g_strcmp0 (ent->d_name, ".."))) {
			continue;
		}

		fd = atoi (ent->d_name);
		if (fd < 3) {
			/* ignore stdandard fds */
			continue;
		}

		if (fds) {
			uint i;
			for (i = 0; i < fds->len; i++) {
				if (g_array_index (fds, int, i) == fd) {
					break;
				}
			}
			if (i != fds->len) {
				continue;
			}
		}

		if (fd == dirfd (dir)) {
			/* ignore the fd created by opendir(3) */
			continue;
		}

		(void)close (fd);
	}

	if (closedir (dir) < 0) {
		return false;
	}

	return true;
}

/*! Perform setup on spawned child process.
 *
 * \param config \ref cc_oci_config.
 *
 * \return \c true on success, else \c false.
 */
static gboolean
cc_oci_setup_child (struct cc_oci_config *config)
{
	/* become session leader */
	setsid ();

	/* Do not close fds when VM runs in detached mode*/
	if (! config->detached_mode) {
		cc_oci_close_fds (NULL);
	}

	cc_oci_setup_hypervisor_logs(config);

	return true;
}

/*! Perform setup on spawned shim process.
 *
 * \param proxy_fd Proxy socket connection 
 * \param proxy_io_fd Proxy IO fd
 *
 * \return \c true on success, else \c false.
 */
static gboolean
cc_oci_setup_shim (int proxy_fd, int proxy_io_fd)
{
	GArray *fds;

	/* become session leader */
	setsid ();

	fds = g_array_sized_new(FALSE, FALSE, sizeof(int), 2);
	g_array_append_val (fds, proxy_fd);
	g_array_append_val (fds, proxy_io_fd);

	cc_oci_close_fds (fds);

	g_array_free(fds, TRUE);

	return true;
}

/*!
 * Close spawned container and stop the main loop.
 *
 * \param pid Process ID.
 * \param status status of child process.
 * \param[out] exit_code exit code of child process.
 *
 * \note \p exit_code may be \c NULL to avoid returning the
 * child exit code.
 */
#if 0
static void
cc_oci_child_watcher (GPid pid, gint status,
		gpointer exit_code) {
	g_debug ("pid %u ended with exit status %d", pid, status);

	if (exit_code) {
		*((gint*)exit_code) = status;
	}

	g_spawn_close_pid (pid);

	g_main_loop_quit (main_loop);
}
#endif

/*!
 * Close spawned hook and stop the hook loop.
 *
 * \param pid Process ID.
 * \param status status of child process.
 * \param[out] exit_code exit code of child process.
 * */
static void
cc_oci_hook_watcher(GPid pid, gint status, gpointer exit_code) {
	g_debug ("Hook pid %u ended with exit status %d", pid, status);

	*((gint*)exit_code) = status;

	g_spawn_close_pid (pid);

	g_main_loop_quit(hook_loop);
}

/*!
 * Handle processes output streams
 * \param channel GIOChannel
 * \param cond GIOCondition
 * \param stream STDOUT_FILENO or STDERR_FILENO
 *
 * \return \c true on success, else \c false.
 * */
static gboolean
cc_oci_output_watcher(GIOChannel* channel, GIOCondition cond,
                            gint stream)
{
	gchar* string = NULL;
	gsize size;

	if (cond == G_IO_HUP) {
		return false;
	}

	g_io_channel_read_line(channel, &string, &size, NULL, NULL);


	if (STDOUT_FILENO == stream) {
		g_message("%s", string ? string : "");
	} else {
		g_warning("%s", string ? string : "");
	}
	g_free_if_set (string);

	return true;
}

/*!
 * Start a hook
 *
 * \param hook \ref oci_cfg_hook.
 * \param state container state.
 * \param state_length length of container state.
 *
 * \return \c true on success, else \c false.
 * */
private gboolean
cc_run_hook(struct oci_cfg_hook* hook, const gchar* state,
             gsize state_length) {
	GError* error = NULL;
	gboolean ret = false;
	gboolean result = false;
	gchar **args = NULL;
	guint args_len = 0;
	GPid pid = 0;
	gint std_in = -1;
	gint std_out = -1;
	gint std_err = -1;
	gint hook_exit_code = -1;
	GIOChannel* out_ch = NULL;
	GIOChannel* err_ch = NULL;
	gchar* container_state = NULL;
	size_t i;
	GSpawnFlags flags = 0x0;

	if (! (hook && state && state_length)) {
		return false;
	}

	if (! hook_loop) {
		/* all the hooks share the same loop */
		return false;
	}

	flags |= G_SPAWN_DO_NOT_REAP_CHILD;
	flags |= G_SPAWN_CLOEXEC_PIPES;

	/* This flag is required to support exec'ing a command that
	 * operates differently depending on its name (argv[0).
	 *
	 * Docker currently uses this, for example, to handle network
	 * setup where it sets:
	 *
	 * path: "/path/to/dockerd"
	 * args: [ "libnetwork-setkey", "$hash_1", "$hash2" ]
	 */
	flags |= G_SPAWN_FILE_AND_ARGV_ZERO;

	if (hook->args) {
		/* command name + args
		 * (where args[0] might not match hook->path).
		 */
		args_len = 1 + g_strv_length(hook->args);
	} else  {
		/* command name + command name.
		 *
		 * Note: The double command name is required since we
		 * must use G_SPAWN_FILE_AND_ARGV_ZERO.
		 */
		args_len = 2;
	}

	/* +1 for for NULL terminator */
	args = g_new0(gchar*, args_len + 1);

	/* args[0] must be file's path */
	args[0] = hook->path;

	/* copy hook->args to args */
	if (hook->args) {
		for (i=0; i<args_len; ++i) {
			args[i+1] = hook->args[i];
		}
	} else {

		/* args[1] is the start of the array passed to the
		 * program specified by args[0] so should contain the
		 * program name (since this will become argv[0] for the
		 * exec'd process).
		 */
		args[1] = hook->path;
	}

	g_debug ("running hook command '%s' as:", args[0]);

	/* +1 to ignore the original (hook->path) name since:
	 *
	 * 1) It's just been displayed above.
	 * 2) Although the command to exec is args[0],
	 *    it will be known as args[1].
	 */
	for (gchar** p = args+1; p && *p; p++) {
		g_debug ("arg: '%s'", *p);
	}

	ret = g_spawn_async_with_pipes(NULL,
			args,
			hook->env,
			flags,
			NULL,
			NULL,
			&pid,
			&std_in,
			&std_out,
			&std_err,
			&error
	);

	/* check errors */
	if (!ret) {
		g_critical("failed to spawn hook");
		if (error) {
			g_critical("error: %s", error->message);
		}
		goto exit;
	}

	g_debug ("hook process ('%s') running with pid %d",
			args[0], (int)pid);

	/* add watcher to hook */
	g_child_watch_add(pid, cc_oci_hook_watcher, &hook_exit_code);

	/* create output channels */
	out_ch = g_io_channel_unix_new(std_out);
	err_ch = g_io_channel_unix_new(std_err);

	/* add watchers to channels */
	if (out_ch) {
		g_io_add_watch(out_ch, G_IO_IN | G_IO_HUP,
			(GIOFunc)cc_oci_output_watcher, (gint*)STDOUT_FILENO);
	} else {
		g_critical("failed to create out channel");
	}
	if (err_ch) {
		g_io_add_watch(err_ch, G_IO_IN | G_IO_HUP,
			(GIOFunc)cc_oci_output_watcher, (gint*)STDERR_FILENO);
	} else {
		g_critical("failed to create err channel");
	}

	/* write container state to hook's stdin */
	container_state = g_strdup(state);
	g_strdelimit(container_state, "\n", ' ');
	if (write(std_in, container_state, state_length) < 0) {
		g_critical ("failed to send container state to hook: %s",
				strerror (errno));
		goto exit;
	}

	/* commit container state to stdin */
	if (write(std_in, "\n", 1) < 0) {
		g_critical ("failed to commit container state: %s",
				strerror (errno));
		goto exit;
	}

	/* required to complete notification to hook */
	close (std_in);

	/* (re-)start the main loop and wait for the hook
	 * to finish.
	 */

	g_main_loop_run(hook_loop);

	/* check hook exit code */
	if (hook_exit_code != 0) {
		g_critical("hook process %d failed with exit code: %d",
				(int)pid,
				hook_exit_code);
		goto exit;
	}

	g_debug ("hook process %d finished successfully", (int)pid);

	result = true;

exit:
	g_free_if_set(container_state);
	g_free_if_set(args);
	if (error) {
		g_error_free(error);
	}

	/* Note: this call does not close the fd */
	if (out_ch) g_io_channel_unref (out_ch);
	if (err_ch) g_io_channel_unref (err_ch);

	if (std_out != -1) close (std_out);
	if (std_err != -1) close (std_err);

	return result;
}


/*!
 * Obtain the network configuration by querying the network namespace.
 *
 * \param[in,out] config \ref cc_oci_config.
 * \param hndl handle returned from a call to \ref netlink_init().
 *
 * \return \c true on success, else \c false.
 */
static gboolean
cc_oci_vm_netcfg_get (struct cc_oci_config *config,
		      struct netlink_handle *hndl)
{
	if (!config) {
		return false;
	}

	/* TODO: We need to support multiple networks */
	if (!cc_oci_network_discover (config, hndl)) {
		g_critical("Network discovery failed");
		return false;
	}

	return true;
}

/*!
 * Create a socket connection from a fd.
 *
 * \param fd int.
 *
 * \return a GSocketConnection on success, else NULL.
 */
static GSocketConnection *
socket_connection_from_fd (int fd)
{
	GError            *error = NULL;
	GSocket           *socket = NULL;
	GSocketConnection *connection = NULL;

	if( fd < 0 ) {
		goto out;
	}

	socket = g_socket_new_from_fd (fd, &error);
	if (! socket) {
		g_critical("failed to create socket from fd");
		if (error) {
			g_critical("%s", error->message);
			g_error_free(error);
		}
		goto out;
	}

	g_socket_set_blocking (socket, TRUE);

	connection = g_socket_connection_factory_create_connection (socket);
	if (!connection) {
		g_critical("failed to create socket connection from fd");
	}

out:
	if (socket) {
		g_object_unref (socket);
	}

	return connection;
}

/*!
 * Start \ref CC_OCI_SHIM as a child process.
 *
 * \param config \ref cc_oci_config.
 * \param child_err_fd Readable file descriptor caller should use
 *   to determine if the shim launched successfully
 *   (any data that can be read from this fd denotes failure).
 * \param shim_args_fd Writable file descriptor caller should use to
 *   send the proxy_socket_fd to the child before \ref CC_OCI_SHIM is
 *   launched.
 * \param shim_socket_fd Writable file descriptor caller should use to
 *   send the proxy IO fd to child before \ref CC_OCI_SHIM is launched.
 *
 * \return \c true on success, else \c false.
 */
static gboolean
cc_shim_launch (struct cc_oci_config *config,
		int *child_err_fd,
		int *shim_args_fd,
		int *shim_socket_fd)
{
	gboolean  ret = false;
	GPid      pid;
	int       child_err_pipe[2] = {-1, -1};
	int       shim_args_pipe[2] = {-1, -1};
	int       shim_socket[2] = {-1, -1};

	if (! (config && config->proxy)) {
		return false;
	}

	if (! (child_err_fd && shim_args_fd && shim_socket_fd)) {
		return false;
	}

	if (pipe2 (child_err_pipe, O_CLOEXEC) < 0) {
		g_critical ("failed to create shim err pipe: %s",
				strerror (errno));
		goto out;
	}

	if (pipe2 (shim_args_pipe, O_CLOEXEC) < 0) {
		g_critical ("failed to create shim args pipe: %s",
				strerror (errno));
		goto out;
	}

	if (socketpair(PF_UNIX, SOCK_STREAM, 0, shim_socket) < 0) {
		g_critical ("failed to create shim socket: %s",
				strerror (errno));
		goto out;
	}

	/* Inform caller of workload PID */
	pid = config->state.workload_pid = fork ();

	if (pid < 0) {
		g_critical ("failed to spawn shim child: %s",
				strerror (errno));
		goto out;
	} else if (! pid) {
		gchar   **args = NULL;
		ssize_t   bytes;
		int       proxy_socket_fd = -1;
		int       proxy_io_fd = -1;
		int       proxy_io_base = -1;
		GSocketConnection *connection = NULL;
		GError   *error = NULL;
		int       flags;

		/* child */

		/* inform the child who they are */
		config->state.workload_pid = getpid ();

		close (child_err_pipe[0]);
		close (shim_args_pipe[1]);
		close (shim_socket[1]);

		g_debug ("shim child waiting for proxy socket fd on fd %d", shim_args_pipe[0]);

		/* block reading proxy fd */
		bytes = read (shim_args_pipe[0],
				&proxy_socket_fd,
				sizeof (proxy_socket_fd));

		if (bytes <= 0) {
			g_critical ("failed to read proxy socket fd");
			goto child_failed;
		}

		/* block reading proxy ioBase */
		bytes = read (shim_args_pipe[0],
				&proxy_io_base,
				sizeof (proxy_io_base));

		if (bytes <= 0) {
			g_critical ("failed to read proxy ioBase");
			goto child_failed;
		}

		/* read proxy IO fd from socket out-of-band */
		connection = socket_connection_from_fd(shim_socket[0]);
		if (!connection) {
			g_critical ("failed to read proxy IO fd");
			goto child_failed;
		}

		proxy_io_fd = g_unix_connection_receive_fd (
			G_UNIX_CONNECTION (connection), NULL, &error);
		if (proxy_io_fd < 0) {
			g_critical ("failed to read proxy IO fd from socket");
			if (error) {
				g_critical("%s", error->message);
				g_error_free(error);
			}
			goto child_failed;
		}

		close (shim_args_pipe[0]);
		shim_args_pipe[0] = -1;

		close (shim_socket[0]);
		shim_socket[0] = -1;

		g_debug ("proxy socket fd from parent=%d",
				proxy_socket_fd);

		if (proxy_socket_fd < 0) {
			g_critical ("parent provided invalid proxy fd");
			goto child_failed;
		}

		/* remove FD_CLOEXEC flags */
		flags = fcntl(proxy_socket_fd, F_GETFD);
		flags &= ~FD_CLOEXEC;
		fcntl(proxy_socket_fd, F_SETFD, flags);

		/* remove FD_CLOEXEC flags */
		flags = fcntl(proxy_io_fd, F_GETFD);
		flags &= ~FD_CLOEXEC;
		fcntl(proxy_io_fd, F_SETFD, flags);

		/* +1 for for NULL terminator */
		args = g_new0 (gchar *, 11+1);
		args[0] = g_strdup (CC_OCI_SHIM);
		args[1] = g_strdup ("-c");
		args[2] = g_strdup (config->optarg_container_id);
		args[3] = g_strdup ("-p");
		args[4] = g_strdup_printf ("%d", proxy_socket_fd);
		args[5] = g_strdup ("-o");
		args[6] = g_strdup_printf ("%d", proxy_io_fd);
		args[7] = g_strdup ("-s");
		args[8] = g_strdup_printf ("%d", proxy_io_base);
		args[9] = g_strdup ("-e");
		args[10] = g_strdup_printf ("%d", proxy_io_base + 1);

		g_debug ("running command:");
		for (gchar** p = args; p && *p; p++) {
			g_debug ("arg: '%s'", *p);
		}

		if (! cc_oci_setup_shim (proxy_socket_fd, proxy_io_fd)) {
			goto child_failed;
		}

		if (execvp (args[0], args) < 0) {
			g_critical ("failed to exec child %s: %s",
					args[0],
					strerror (errno));
			abort ();
		}

child_failed:
		/* Any data written by the child to this pipe signifies failure,
		 * so send a very short message ("E", denoting Error).
		 */
		close (shim_args_pipe[0]);
		shim_args_pipe[0] = -1;

		close (shim_socket[0]);
		shim_socket[0] = -1;

		(void)write (child_err_pipe[1], "E", 1);
		exit (EXIT_FAILURE);
	}

	/* parent */

	g_debug ("shim process running with pid %d", (int)pid);

	*child_err_fd = child_err_pipe[0];
	*shim_args_fd = shim_args_pipe[1];
	*shim_socket_fd = shim_socket[1];

	ret = true;

out:
	close (child_err_pipe[1]);
	close (shim_args_pipe[0]);
	close (shim_socket[0]);

	return ret;
}

/*!
 * Start the hypervisor as a child process.
 *
 * Due to the way networking is handled in Docker, the logic here
 * is unfortunately rather complex.
 *
 * \param config \ref cc_oci_config.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_vm_launch (struct cc_oci_config *config)
{
	gboolean           ret = false;
	GPid               pid;
	ssize_t            bytes;
	char               buffer[2] = { '\0' };
	int                hypervisor_args_pipe[2] = {-1, -1};
	int                child_err_pipe[2] = {-1, -1};
	gchar            **args = NULL;
	gchar            **p;
	g_autofree gchar  *timestamp = NULL;
	struct netlink_handle *hndl = NULL;
	gboolean           setup_networking;
	gboolean           hook_status = false;
	GPtrArray         *additional_args = NULL;
	gint               hypervisor_args_len;
	g_autofree gchar  *hypervisor_args = NULL;
	int                shim_err_fd = -1;
	int                shim_args_fd = -1;
	int                shim_socket_fd = -1;
	int                proxy_fd = -1;
	int                proxy_io_fd = -1;
	int                ioBase = -1;
	GSocketConnection *shim_socket_connection = NULL;
	GError            *error = NULL;

	if (! config) {
		return false;
	}

	setup_networking = cc_oci_enable_networking ();

	timestamp = cc_oci_get_iso8601_timestamp ();
	if (! timestamp) {
		goto out;
	}

        additional_args = g_ptr_array_new ();

	config->state.status = OCI_STATUS_CREATED;

	/* The namespace setup occurs in the parent to ensure
	 * the hooks run successfully. The child will automatically
	 * inherit the namespaces.
	 */
	if (! cc_oci_ns_setup (config)) {
		goto out;
	}

	/* Connect to the proxy before launching the shim so that the
	 * proxy socket fd can be passed to the shim.
	 */
	if (! cc_proxy_connect (config->proxy)) {
		return false;
	}

	/* Set up comms channels to the child:
	 *
	 * - one to pass the full list of expanded hypervisor arguments.
	 *
	 * - one to allow detection of successful child setup: if
	 *   the child closes the pipe, it was successful, but if it
	 *   writes data to the pipe, setup failed.
	 */

	if (pipe2 (child_err_pipe, O_CLOEXEC) < 0) {
		g_critical ("failed to create child error pipe: %s",
				strerror (errno));
		goto out;
	}

	if (pipe2 (hypervisor_args_pipe, O_CLOEXEC) < 0) {
		g_critical ("failed to create hypervisor args pipe: %s",
				strerror (errno));
		goto out;
	}

	pid = config->vm->pid = fork ();
	if (pid < 0) {
		g_critical ("failed to create child: %s",
				strerror (errno));
		goto out;
	}

	if (! pid) {
		/* child */

		/* inform the child who they are */
		config->vm->pid = getpid ();

		close (hypervisor_args_pipe[1]);
		close (child_err_pipe[0]);

		/* The child doesn't need the proxy connection */
		if (! cc_proxy_disconnect (config->proxy)) {
			goto child_failed;
		}

		/* first - read hypervisor args length */
		g_debug ("reading hypervisor command-line length from pipe");
		bytes = read (hypervisor_args_pipe[0], &hypervisor_args_len,
			sizeof (hypervisor_args_len));
		if (bytes < 0) {
			g_critical ("failed to read hypervisor args length");
			goto child_failed;
		}

		hypervisor_args = g_new0(gchar, (gsize)(1+hypervisor_args_len));
		if (! hypervisor_args) {
			g_critical ("failed alloc memory for hypervisor args");
			goto child_failed;
		}

		/* second - read hypervisor args */
		g_debug ("reading hypervisor command-line from pipe");
		bytes = read (hypervisor_args_pipe[0], hypervisor_args,
			(size_t)hypervisor_args_len);
		if (bytes < 0) {
			g_critical ("failed to read hypervisor args");
			goto child_failed;
		}

		/* third - convert string to args, the string that was read
		 * from pipe has '\n' as delimiter
		 */
		args = g_strsplit_set(hypervisor_args, "\n", -1);
		if (! args) {
			g_critical ("failed split hypervisor args");
			goto child_failed;
		}

		if (setup_networking) {
			hndl = netlink_init();
			if (hndl == NULL) {
				g_critical("failed to setup netlink socket");
				goto child_failed;
			}

			if (! cc_oci_vm_netcfg_get (config, hndl)) {
				g_critical("failed to discover network configuration");
				goto child_failed;
			}

			if (! cc_oci_network_create(config, hndl)) {
				g_critical ("failed to create network");
				goto child_failed;
			}
			g_debug ("network configuration complete");

		}

		g_debug ("running command:");
		for (p = args; p && *p; p++) {
			g_debug ("arg: '%s'", *p);
		}

		if (! cc_oci_setup_child (config)) {
			goto child_failed;
		}

		if (execvp (args[0], args) < 0) {
			g_critical ("failed to exec child %s: %s",
					args[0],
					strerror (errno));
			abort ();
		}

child_failed:
		/* Any data written by the child to this pipe signifies failure,
		 * so send a very short message ("E", denoting Error).
		 */
		(void)write (child_err_pipe[1], "E", 1);
		exit (EXIT_FAILURE);
	}

	/* parent */

	g_debug ("hypervisor child pid is %u", (unsigned)pid);

	/* Before fork this process again
	 * we have to close unused file descriptors
	 */
	close (hypervisor_args_pipe[0]);
	hypervisor_args_pipe[0] = -1;

	close (child_err_pipe[1]);
	child_err_pipe[1] = -1;

	/* Launch the shim child before the state file is created.
	 *
	 * Required since the state file must contain the workloads pid,
	 * and for our purposes the workload pid is the pid of the shim.
	 *
	 * The child blocks waiting for a write to shim_args_fd.
	 */
	if (! cc_shim_launch (config, &shim_err_fd, &shim_args_fd, &shim_socket_fd)) {
		goto out;
	}

	/* Create state file before hooks run.
	 *
	 * Required since the hooks must be passed the runtime state.
	 *
	 * XXX: Note that at this point, although the workload PID
	 * is set (which satisfies the OCI state file requirements),
	 * there are no proxy details that can be added to the state
	 * file. For this reason, the state file is recreated (with full
	 * details) later.
	 */
	ret = cc_oci_state_file_create (config, timestamp);
	if (! ret) {
		g_critical ("failed to create state file");
		goto out;
	}

	/* Run the pre-start hooks.
	 *
	 * Note that one of these hooks will configure the networking
	 * in the network namespace.
	 *
	 * If a hook returns a non-zero exit code, then an error
	 * including the exit code and the stderr is returned to
	 * the caller and the container is torn down.
	 */
	hook_status = cc_run_hooks (config->oci.hooks.prestart,
			config->state.state_file_path,
			true);

	if (! hook_status) {
		g_critical ("failed to run prestart hooks");
	}

	g_debug ("building hypervisor command-line");

	// FIXME: add network config bits to following functions:
	//
	// - cc_oci_container_state()
	// - oci_state()
	// - cc_oci_update_options()

	cc_oci_populate_extra_args(config, &additional_args);
	ret = cc_oci_vm_args_get (config, &args, additional_args);
	if (! (ret && args)) {
		goto out;
	}

	hypervisor_args = g_strjoinv("\n", args);
	if (! hypervisor_args) {
		g_critical("failed to join hypervisor args");
		goto out;
	}

	hypervisor_args_len = (gint)g_utf8_strlen(hypervisor_args, -1);

	/* first - write hypervisor length */
	bytes = write (hypervisor_args_pipe[1], &hypervisor_args_len,
		sizeof(hypervisor_args_len));
	if (bytes < 0) {
		g_critical ("failed to send hypervisor args length to child: %s",
			strerror (errno));
		goto out;
	}

	/* second - write hypervisor args */
	bytes = write (hypervisor_args_pipe[1], hypervisor_args,
		(size_t)hypervisor_args_len);
	if (bytes < 0) {
		g_critical ("failed to send hypervisor args to child: %s",
			strerror (errno));
		goto out;
	}

	g_debug ("checking child setup (blocking)");

	/* block reading child error state */
	bytes = read (child_err_pipe[0],
			buffer,
			sizeof (buffer));
	if (bytes > 0) {
		g_critical ("child setup failed");
		ret = false;
		goto out;
	}

	g_debug ("child setup successful");

	/* Wait for the proxy to signal readiness.
	 *
	 * This can only happen once the agent details have been added
	 * to the proxy object.
	 */
	if (! cc_proxy_wait_until_ready (config)) {
		g_critical ("failed to wait for proxy %s", CC_OCI_PROXY);
		goto out;
	}

	/* At this point ctl and tty sockets already exist,
	 * is time to communicate with the proxy
	 */
	if (! cc_proxy_hyper_pod_create (config)) {
		goto out;
	}

	proxy_fd = g_socket_get_fd (config->proxy->socket);
	if (proxy_fd < 0) {
		g_critical ("invalid proxy fd: %d", proxy_fd);
		goto out;
	}

	bytes = write (shim_args_fd, &proxy_fd, sizeof (proxy_fd));
	if (bytes < 0) {
		g_critical ("failed to send proxy fd to shim child: %s",
			strerror (errno));
		goto out;
	}

	if (! cc_proxy_cmd_allocate_io(config->proxy,
			&proxy_io_fd, &ioBase)) {
		goto out;
	}

	bytes = write (shim_args_fd, &ioBase, sizeof (ioBase));
	if (bytes < 0) {
		g_critical ("failed to send proxy ioBase to shim child: %s",
			strerror (errno));
		goto out;
	}

	/* send proxy IO fd to cc-shim child */
	shim_socket_connection = socket_connection_from_fd(shim_socket_fd);
	if (! shim_socket_connection) {
		g_critical("failed to create a socket connection to send proxy IO fd");
		goto out;
	}

	ret = g_unix_connection_send_fd (G_UNIX_CONNECTION (shim_socket_connection),
		proxy_io_fd, NULL, &error);

	if (! ret) {
		g_critical("failed to send proxy IO fd");
		goto out;
	}

	/* save ioBase */
	config->oci.process.stdio_stream = ioBase;
	config->oci.process.stderr_stream = ioBase + 1;

	close (shim_args_fd);
	shim_args_fd = -1;

	g_debug ("checking shim setup (blocking)");

	bytes = read (shim_err_fd,
			buffer,
			sizeof (buffer));
	if (bytes > 0) {
		g_critical ("shim setup failed");
		ret = false;
		goto out;
	}

	/* Recreate the state file now that all information is
	 * available.
	 */
	g_debug ("recreating state file");

	ret = cc_oci_state_file_create (config, timestamp);
	if (! ret) {
		g_critical ("failed to recreate state file");
		goto out;
	}

	/* parent can now disconnect from the proxy (but the shim
	 * remains connected).
	 */
	ret = cc_proxy_disconnect (config->proxy);

	/* Finally, create the pid file.
	 *
	 * This MUST be done after all setup since containerd
	 * considers "create" finished when this file has been created
	 * (and it will then goes on to call "start").
	 */
	if (config->pid_file) {
		ret = cc_oci_create_pidfile (config->pid_file,
				config->state.workload_pid);
		if (! ret) {
			goto out;
		}
	}
out:
	if (hypervisor_args_pipe[0] != -1) close (hypervisor_args_pipe[0]);
	if (hypervisor_args_pipe[1] != -1) close (hypervisor_args_pipe[1]);
	if (child_err_pipe[0] != -1) close (child_err_pipe[0]);
	if (child_err_pipe[1] != -1) close (child_err_pipe[1]);
	if (shim_err_fd != -1) close (shim_err_fd);
	if (shim_args_fd != -1) close (shim_args_fd);
	if (shim_socket_fd != -1) close (shim_socket_fd);

	if (setup_networking) {
		netlink_close (hndl);
	}

	if (args) {
		g_strfreev (args);
	}
	if (additional_args) {
		g_ptr_array_free(additional_args, TRUE);
	}

	return ret;
}

/*!
 * Run hooks.
 *
 * \param hooks \c GSList.
 * \param state_file_path Full path to state file.
 * \param stop_on_failure Stop on error if \c true.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_run_hooks(GSList* hooks, const gchar* state_file_path,
              gboolean stop_on_failure) {
	GSList* i = NULL;
	struct oci_cfg_hook* hook = NULL;
	gchar* container_state = NULL;
	gsize length = 0;
	GError* error = NULL;
	gboolean result = false;

	/* no hooks */
	if ((!hooks) || g_slist_length(hooks) == 0) {
		return true;
	}

	/* create a new main loop */
	hook_loop = g_main_loop_new(NULL, 0);
	if (!hook_loop) {
		g_critical("cannot create a new main loop\n");
		return false;
	}

	/* The state of the container is passed to the hooks over stdin,
	 * so the hooks could get the information they need to do their
	 * work.
	 */
	if (! g_file_get_contents(state_file_path, &container_state,
	        &length, &error)) {
		g_critical ("failed to read state file: %s",
		    error->message);
		g_error_free(error);
		goto exit;
	}

	for (i=g_slist_nth(hooks, 0); i; i=g_slist_next(i) ) {
		hook = (struct oci_cfg_hook*)i->data;
		if ((!cc_run_hook(hook, container_state, length)) && stop_on_failure) {
			goto exit;
		}
	}

	result = true;

exit:
	g_free_if_set(container_state);
	g_main_loop_unref(hook_loop);
	hook_loop = NULL;

	return result;
}

/*!
 * Create a connection to the VM, run a command and disconnect.
 *
 * \param config \ref cc_oci_config.
 * \param argc Argument count.
 * \param argv Argument vector.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_vm_connect (struct cc_oci_config *config,
		int argc,
		char *const argv[]) {
	gchar     **args = NULL;
	gboolean    ret = false;
	guint       args_len = 0;
	gboolean    cmd_is_just_shell = false;
#if 0
	GError     *err = NULL;
	GPid        pid;
	gint        exit_code = -1;
	GSpawnFlags flags = (G_SPAWN_CHILD_INHERITS_STDIN |
			     /* XXX: required for g_child_watch_add! */
			     G_SPAWN_DO_NOT_REAP_CHILD |
			     G_SPAWN_SEARCH_PATH);
	guint i;
#endif

	g_assert (config);
	g_assert (argc);
	g_assert (argv);

	/* Check if the user has specified a shell to run.
	 *
	 * FIXME: This is a pragmatic (but potentially unreliable) solution
	 * FIXME:   if the user wants to run an unknown shell.
	 */
	if (cc_oci_cmd_is_shell (argv[0])) {
		cmd_is_just_shell = true;
	}

	/* The user wants to run an interactive shell.
	 * However, this is the default with ssh if no command is
	 * specified.
	 *
	 * If a shell is passed as the 1st arg, ssh gets
	 * confused as it's expecting to run a non-interactive command,
	 * so simply remove it to get the behaviour the user wants.
	 *
	 * An extra check is performed to ensure that the argument after
	 * the shell is not an option to ensure that commands like:
	 * "bash -c ..." still work as expected.
	 */
	if (argv[1] && argv[1][0] == '-') {
		cmd_is_just_shell = false;
	}

	/* Just a shell, so remove the argument to get the expected
	 * behavior of an interactive shell.
	 */
	if (cmd_is_just_shell) {
		argc--;
		argv++;
	}

	args_len = (guint)argc;

	/* +2 for CC_OCI_EXEC_CMD + hostname to connect to */
	args_len += 2;

	/* +1 for NULL terminator */
	args = g_new0 (gchar *, args_len + 1);
	if (! args) {
		return false;
	}

	/* The command to use to connect to the VM */
	args[0] = g_strdup (CC_OCI_EXEC_CMD);
	if (! args[0]) {
		ret = false;
		goto out;
	}

	/* connection string to connect to the VM */
	// FIXME: replace with proper connection string once networking details available.
#if 0
	args[1] = g_strdup_printf (ip_address);

	/* append argv to the end of args */
	if (argc) {
		for (i = 0; i < args_len; ++i) {
			args[i+2] = g_strdup (argv[i]);
		}
	}

	g_debug ("running command:");
	for (gchar** p = args; p && *p; p++) {
		g_debug ("arg: '%s'", *p);
	}

	/* create a new main loop */
	main_loop = g_main_loop_new (NULL, 0);
	if (! main_loop) {
		g_critical ("cannot create main loop for client");
		goto out;
	}

	ret = g_spawn_async_with_pipes (NULL, /* working directory */
			args,
			NULL, /* inherit parents environment */
			flags,
			(GSpawnChildSetupFunc)cc_oci_close_fds,
			NULL, /* user_data */
			&pid,
			NULL, /* standard_input */
			NULL, /* standard_output */
			NULL, /* standard_error */
			&err);

	if (! ret) {
		g_critical ("failed to spawn child process (%s): %s",
				args[0], err->message);
		g_error_free (err);
		goto out;
	}

	g_debug ("child process ('%s') running with pid %u",
			args[0], (unsigned)pid);

	g_child_watch_add (pid, cc_oci_child_watcher, &exit_code);

	/* wait for child to finish */
	g_main_loop_run (main_loop);

	g_debug ("child pid %u exited with code %d",
			(unsigned )pid, (int)exit_code);

	if (exit_code) {
		/* failed */
		goto out;
	}

	ret = true;
#else
	g_critical ("not implemented yet");
	ret = false;
#endif

out:
	if (args) {
		g_strfreev (args);
	}

	if (main_loop) {
		g_main_loop_unref (main_loop);
		main_loop = NULL;
	}

	return ret;
}
