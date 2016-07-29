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
#include <sys/ptrace.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include <gio/gunixsocketaddress.h>

#include "oci.h"
#include "util.h"
#include "hypervisor.h"
#include "process.h"
#include "namespace.h"
#include "common.h"

static GMainLoop* main_loop = NULL;
static GMainLoop* hook_loop = NULL;

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
 * \return \c true on success, else \c false.
 */
static gboolean
cc_oci_close_fds (void) {
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
 */
static void
cc_oci_setup_child (struct cc_oci_config *config)
{
	/* become session leader */
	setsid ();

	if (! cc_oci_ns_setup (config)) {
		return;
	}

	/* arrange for the process to be paused when the child command
	 * is exec(3)'d to ensure that the VM does not launch until
	 * "start" is called.
	 */
	if (ptrace (PTRACE_TRACEME, 0, NULL, 0) < 0) {
		g_critical ("failed to ptrace in child: %s",
				strerror (errno));
		return;
	}

	/* log before the fds are closed */
	g_debug ("set ptrace on child");

	/* Do not close fds when VM runs in detached mode*/
	if (! config->detached_mode) {
		cc_oci_close_fds ();
	}

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
	gchar* string;
	gsize size;

	if (cond == G_IO_HUP) {
		g_io_channel_unref(channel);
		return false;
	}

	g_io_channel_read_line(channel, &string, &size, NULL, NULL);
	if (STDOUT_FILENO == stream) {
		g_message("%s", string);
	} else {
		g_warning("%s", string);
	}
	g_free(string);

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
static gboolean
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

	if (! hook_loop) {
		/* all the hooks share the same loop */
		return false;
	}

	if (hook->args) {
		/* command name + args */
		args_len = 1 + g_strv_length(hook->args);
	} else  {
		/* command name only */
		args_len = 1;
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
	}

	flags |= G_SPAWN_DO_NOT_REAP_CHILD;
	flags |= G_SPAWN_CLOEXEC_PIPES;

	g_debug ("running hook command:");
	for (gchar** p = args; p && *p; p++) {
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

	if (std_out != -1) close (std_out);
	if (std_err != -1) close (std_err);

	return result;
}

/*!
 * Start the hypervisor (in a paused state) as a child process.
 *
 * \param config \ref cc_oci_config.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_vm_launch (struct cc_oci_config *config)
{
	gchar     **args = NULL;
	GError     *err = NULL;
	gboolean    ret;
	GPid        pid;
	int         status = 0;
	GSpawnFlags flags = 0x0;
	gchar     **p;

	ret = cc_oci_vm_args_get (config, &args);
	if (! (ret && args)) {
		return ret;
	}

	flags |= G_SPAWN_DO_NOT_REAP_CHILD;
	flags |= G_SPAWN_CLOEXEC_PIPES;

	/* discard stderr and stdout to avoid hangs */
	flags |= (G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL);

	g_debug ("running command:");
	for (p = args; p && *p; p++) {
		g_debug ("arg: '%s'", *p);
	}

	ret = g_spawn_async_with_pipes(config->oci.root.path,
			args,
			NULL, /* inherit parents environment */
			flags,
			(GSpawnChildSetupFunc)cc_oci_setup_child,
			(gpointer)config,
			&config->state.workload_pid,
			NULL,
			NULL,
			NULL,
			&err);

	if (! ret) {
		g_critical ("failed to spawn child: %s", err->message);
		g_error_free (err);
		goto out;
	}

	pid = config->state.workload_pid;

	/* wait for child to receive the expected SIGTRAP caused
	 * by it calling exec(2) whilst under PTRACE control.
	 */
	if (waitpid (pid, &status, 0) != pid) {
		g_critical ("failed to wait for child %d: %s",
				pid, strerror (errno));
		goto out;
	}

	if (! WIFSTOPPED (status)) {
		g_critical ("child %d not stopped by signal", pid);
		goto out;
	}

	if (! (WSTOPSIG (status) == SIGTRAP)) {
		g_critical ("child %d not stopped by expected signal",
				pid);
		goto out;
	}

	/* Stop tracing, but send a stop signal to the child so that it
	 * remains in a paused state.
	 */
	if (ptrace (PTRACE_DETACH, pid, NULL, SIGSTOP) < 0) {
		g_critical ("failed to ptrace detach in child %d: %s",
				(int)pid,
				strerror (errno));
		goto out;
	}

	g_debug ("child process ('%s') running in stopped state "
			"with pid %u",
		       	args[0],
			(unsigned)config->state.workload_pid);

	if (config->pid_file) {
		ret = cc_oci_create_pidfile (config->pid_file,
				config->state.workload_pid);
	} else {
		ret = true;
	}

out:
	g_strfreev (args);

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
	GError     *err = NULL;
	gboolean    ret = false;
	GPid        pid;
	guint       args_len = 0;
	gint        exit_code = -1;
	gboolean    cmd_is_just_shell = false;
	GSpawnFlags flags = (G_SPAWN_CHILD_INHERITS_STDIN |
			     /* XXX: required for g_child_watch_add! */
			     G_SPAWN_DO_NOT_REAP_CHILD |
			     G_SPAWN_SEARCH_PATH);
	guint i;

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
#else
	g_critical ("not implemented yet");
	ret = false;
	goto out;
#endif

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
