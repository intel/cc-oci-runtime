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
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

#include "oci.h"
#include "util.h"
#include "logging.h"
#include "common.h"

/** Flags to pass to \c g_log_set_handler(). */
#define CLR_OCI_LOG_FLAGS \
(                                                                 \
 (G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION ) | \
                                                                  \
 G_LOG_LEVEL_CRITICAL |                                           \
 G_LOG_LEVEL_WARNING  |                                           \
 G_LOG_LEVEL_MESSAGE  |                                           \
 G_LOG_LEVEL_INFO     |                                           \
 G_LOG_LEVEL_DEBUG                                                \
)

/** Size of buffer for logging. */
#define CLR_OCI_LOG_BUFSIZE 1024

/** Fallback logging for catastrophic failures. */
#define CLR_OCI_ERROR(...) \
	clr_oci_error (__FILE__, __LINE__, __func__, __VA_ARGS__)

extern gboolean enable_debug;

/*!
 * Last-ditch logging routine which sends an error
 * message to syslog.
 *
 * This function is called by \ref clr_oci_log_handler()
 * when it detects an error which stops it from writing to the standard
 * log file.
 *
 * \param file Name of logfile.
 * \param line_number Call site line number.
 * \param function Function at call site.
 * \param fmt Format and arguments to log.
 */
private void
clr_oci_error (const char *file,
		int line_number,
		const char *function,
		const char *fmt,
		...)
{
	gchar    buffer[CLR_OCI_LOG_BUFSIZE];
	va_list  ap;
	int      ret;

	g_assert (file);
	g_assert (line_number >= 0);
	g_assert (function);
	g_assert (fmt);

	va_start (ap, fmt);

	/* detect overrun */
	buffer[CLR_OCI_LOG_BUFSIZE-1] = '\0';

	ret = g_vsnprintf (buffer, CLR_OCI_LOG_BUFSIZE, fmt, ap);
	if (ret < 0) {
		syslog (LOG_ERR, "ERROR: %s: %d: %s: "
				"failed to prepare log buffer "
				"(fmt=\"%s\")\n",
				file,
				line_number,
				function,
				fmt);
		goto out;
	} else if (buffer[CLR_OCI_LOG_BUFSIZE-1] != '\0') {
		syslog (LOG_ERR, "BUG: %s: %d: %s: "
				"buffer is too small "
				"(%d) (fmt=\"%s\")\n",
				file,
				line_number,
				function,
				CLR_OCI_LOG_BUFSIZE,
				fmt);
		goto out;
	}

	syslog (LOG_ERR, "%s:%d:%s:%s",
			file,
			line_number,
			function,
			buffer);

out:
	va_end (ap);

	closelog ();
}

/*!
 * Generate a log message in JSON format.
 *
 * \param timestamp ISO-8601 timestamp to use for log.
 * \param level Log level.
 * \param message Message to log.
 *
 * \return Newly-allocated string containing entry suitable for logging.
 */
static gchar *
clr_oci_log_to_json (const gchar *timestamp,
		const gchar *level,
		const gchar *message)
{
	JsonObject  *obj;
	gchar       *data;
	gsize        data_len;

	g_assert (timestamp);
	g_assert (level);
	g_assert (message);

	obj = json_object_new ();

	/* Add elements */
	json_object_set_string_member (obj, "level", level);
	json_object_set_string_member (obj, "mesg", message);
	json_object_set_string_member (obj, "time", timestamp);

	data = clr_oci_json_obj_to_string (obj, false, &data_len);
	json_object_unref (obj);

	return data;
}

/*!
 * Construct a log message.
 *
 * \param log_domain glib log domain.
 * \param log_level \c G_LOG_LEVEL_*.
 * \param message Text to log.
 * \param timestamp ISO-8601 timestamp to use for log.
 * \param use_json If \c true, log in JSON, else log in ASCII.
 *
 * \return Newly-allocated string containing entry suitable for logging,
 * or \c NULL on error.
 */
static char *
clr_oci_msg_fmt (const gchar *log_domain,
		const gchar *log_level,
		const char *message,
		const char *timestamp,
		gboolean use_json)
{
	gchar  *final = NULL;

	g_assert (message);
	g_assert (timestamp);
	g_assert (log_level);

	if (use_json) {
		gchar *str = NULL;
		str = clr_oci_log_to_json (timestamp, log_level, message);
		if (str) {
			final = g_strdup_printf ("%s\n", str);
			g_free (str);
		}
	} else {
		final = g_strdup_printf ("%s:%u:%s:%s:%s\n",
				timestamp,
				(unsigned)getpid (),
				log_domain ? log_domain : "",
				log_level,
				message);
	}

	return final;
}

/*!
 * Write a log message.
 *
 * \warning Note that this function should not call any glib log
 * handling functions (g_debug(), etc) to avoid going recursive.
 *
 * \param filename Full path of file to write message to.
 * \param message Text to write to \p filename.
 *
 * \return \c true on success, else \c false.
 */
static gboolean
clr_oci_log_msg_write (const char *filename, const char *message)
{
	GIOChannel  *channel = NULL;
	GError      *err = NULL;
	GIOStatus    status;
	gboolean     ret = false;
	int          fd;
	int          flags = (O_CREAT | O_APPEND | O_WRONLY | O_CLOEXEC);

	g_assert (filename);
	g_assert (message);

	fd = open (filename, flags, CLR_OCI_LOGFILE_MODE);
	if (fd < 0) {
		CLR_OCI_ERROR ("failed to open logfile %s for writing: %s",
				filename, strerror (errno));
		goto out;
	}

	/* append to file if it already exists */
	channel = g_io_channel_unix_new (fd);
	if (! channel) {
		CLR_OCI_ERROR ("failed to create I/O channel");
		goto out;
	}

	status = g_io_channel_write_chars (
			channel,
			message,
			-1, /* signify it's a string */
			NULL, /* bytes written */
			&err);

	if (status != G_IO_STATUS_NORMAL) {
		CLR_OCI_ERROR ("failed to write to I/O channel: %s",
				err->message);
		g_error_free (err);
		goto out;
	}

	status = g_io_channel_shutdown (channel,
			TRUE, /* flush */
			&err);
	if (status != G_IO_STATUS_NORMAL) {
		CLR_OCI_ERROR ("failed to close I/O channel: %s",
				err->message);
		g_error_free (err);
		goto out;
	}

	ret = true;

out:
	if (channel) {
		g_io_channel_unref (channel);
	}

	if (fd != -1) {
		close (fd);
	}

	return ret;
}

/*!
 * glib log handler (for \c g_debug(), \c g_message(), \c g_warning(),
 * \c g_critical(), etc).
 *
 * Upon success, a message of the following format will be appended to
 * the specified file:
 *
 *     <timestamp>:<pid>:<domain>:<level>:<message>\n
 *
 * If \c use_json=true, the log format will instead be:
 *
 *     "level":@log_level, "msg",@message, "time",<timestamp>
 *
 * In all cases,
 *
 * - \c &lt;timestamp&gt; is a full ISO-8601 date + time.
 *
 * Errors are fatal since it is imperative we are able to log messages,
 * so there is no point in continuing if we can't.
 *
 * \param log_domain glib log domain.
 * \param log_level \c G_LOG_LEVEL_*.
 * \param message Text to log.
 * \param user_data Logging options (in the form of \ref clr_log_options).
 */
static void
clr_oci_log_handler (const gchar *log_domain,
		GLogLevelFlags log_level,
		const gchar *message,
		gpointer user_data)
{
	const gchar                  *level = NULL;;
	gchar                        *final = NULL;
	gchar                        *timestamp = NULL;
	const struct clr_log_options *options;
	static gboolean               initialised = FALSE;
	gboolean                      ret;

	g_assert (message);

	options = (const struct clr_log_options *)user_data;

	g_assert (options);

	if (! (options->filename || options->global_logfile)) {
		/* No log files, so nothing to do */
		return;
	}

	if (log_level == G_LOG_LEVEL_DEBUG && (!enable_debug) &&
			! options->global_logfile) {

		/* By default, g_debug() messages are disabled. However,
		 * if a global logfile is specified, g_debug() calls are
		 * still logged to that logfile.
		 */
		return;
	}

	if (! initialised) {
		int syslog_options = (LOG_CONS | LOG_PID | LOG_PERROR |
				LOG_NOWAIT);

		/* setup the fallback logging */
		openlog (G_LOG_DOMAIN, syslog_options, LOG_LOCAL0);

		initialised = TRUE;
	}

	switch (log_level) {
	case G_LOG_LEVEL_ERROR:
		level = "error";
		break;

	case G_LOG_LEVEL_CRITICAL:
		level = "critical";
		break;

	case G_LOG_LEVEL_WARNING:
		level = "warning";
		break;

	case G_LOG_LEVEL_MESSAGE:
		level = "message";
		break;

	case G_LOG_LEVEL_INFO:
		level = "info";
		break;

	case G_LOG_LEVEL_DEBUG:
		level = "debug";
		break;

	default:
		level = "error";
		break;
	}

	timestamp = clr_oci_get_iso8601_timestamp ();
	if (! timestamp) {
		goto out;
	}

	final = clr_oci_msg_fmt (log_domain, level, message,
			timestamp, options->use_json);
	if (! final) {
		CLR_OCI_ERROR ("failed to format log entry");
		goto out;
	}

	if (log_level == G_LOG_LEVEL_ERROR || log_level == G_LOG_LEVEL_CRITICAL) {
		/* Ensure the message gets across.
		 *
		 * XXX: Note that writing to stderr cannot occur for
		 * other log levels since this would invalidate JSON
		 * output. However, in an error scenario all bets are
		 * off so we do it anyway.
		 */
		fprintf (stderr, "%s\n", final);
	}

	if ((log_level == G_LOG_LEVEL_DEBUG) && (!enable_debug)) {
		/* Debug calls are always added to the global log, but
		 * only added to the main log if debug is enabled.
		 */
		goto update_global_log;
	}

	if (options->filename) {
		ret = clr_oci_log_msg_write (options->filename, final);
		if (! ret) {
			goto out;
		}
	}

update_global_log:
	if (options->global_logfile) {
		/* If we're logging in JSON, switch back to ASCII for
		 * the global log write as we want all the metadata
		 * possible to be logged.
		 */
		if (options->use_json) {
			g_free_if_set (final);

			final = clr_oci_msg_fmt (log_domain, level, message, timestamp, false);
			if (! final) {
				goto out;
			}
		}
		ret = clr_oci_log_msg_write (options->global_logfile,
				final);
		if (! ret) {
			goto out;
		}
	}

out:
	g_free_if_set (timestamp);
	g_free_if_set (final);
}

/*!
 * Initialise logging.
 *
 * \param options \ref clr_log_options.

 * \return \c true on success, else \c false.
 */
gboolean
clr_oci_log_init (const struct clr_log_options *options)
{
	g_assert (options);

	/* Create path to allow global log file to be created */
	if (options->global_logfile) {
		gboolean  ret;
		gchar    *dir;

		dir = g_path_get_dirname (options->global_logfile);
		if (! dir) {
			return false;
		}

		ret = g_mkdir_with_parents (dir, CLR_OCI_DIR_MODE);
		if (ret) {
			g_free (dir);
			return false;
		}

		g_free (dir);
	}

	(void)g_log_set_handler (G_LOG_DOMAIN,
			(GLogLevelFlags)CLR_OCI_LOG_FLAGS,
			clr_oci_log_handler,
			(gpointer)options);

	return true;
}

/**
 *
 * Free resources held by the logging options.
 *
 * \param options \ref clr_log_options.
 */
void
clr_oci_log_free (struct clr_log_options *options)
{
	if (! options) {
		return;
	}

	g_free_if_set (options->filename);
	g_free_if_set (options->global_logfile);
}
