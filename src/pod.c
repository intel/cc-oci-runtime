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

/* Sandbox rootfs */
#define CC_POD_SANDBOX_ROOTFS "workloads"

/* CRI-O/ocid namespaces */
#define CC_POD_OCID_NAMESPACE "ocid/"
#define CC_POD_OCID_NAMESPACE_SIZE 5

#define CC_POD_OCID_CONTAINER_TYPE "ocid/container_type"
#define CC_POD_OCID_SANDBOX        "sandbox"
#define CC_POD_OCID_CONTAINER      "container"

#define CC_POD_OCID_SANDBOX_NAME "ocid/sandbox_name"

#include <errno.h>
#include <string.h>

#include <glib.h>
#include <gio/gunixconnection.h>

#include "pod.h"
#include "process.h"
#include "proxy.h"
#include "state.h"

/**
 * Handle pod related OCI annotations.
 * This routine will build the config->pod structure
 * based on the pod related OCI annotations.
 *
 * \param OCI config \ref cc_oci_config.
 * \param OCI annotation \ref oci_state.
 *
 * \return 0 on success, and a negative \c errno on failure.
 */
int
cc_pod_handle_annotations(struct cc_oci_config *config, struct oci_cfg_annotation *annotation)
{
	if (! (config && annotation)) {
		return -EINVAL;
	}

	if (! (annotation->key && annotation->value)) {
		return -EINVAL;
	}

	/* We only handle CRI-O/ocid annotations for now */
	if (strncmp(annotation->key, CC_POD_OCID_NAMESPACE,
		    CC_POD_OCID_NAMESPACE_SIZE) != 0) {
		return 0;
	}

	if (! config->pod) {
		config->pod = g_malloc0 (sizeof (struct cc_pod));
		if (! config->pod) {
			return -ENOMEM;
		}
	}

	if (g_strcmp0(annotation->key, CC_POD_OCID_CONTAINER_TYPE) == 0) {
		if (g_strcmp0(annotation->value, CC_POD_OCID_SANDBOX) == 0) {
			config->pod->sandbox = true;
			config->pod->sandbox_name = g_strdup(config->optarg_container_id);

			g_snprintf (config->pod->sandbox_workloads,
				    sizeof (config->pod->sandbox_workloads),
				    "%s/%s/%s",
				    CC_OCI_RUNTIME_DIR_PREFIX,
				    config->optarg_container_id,
				    CC_POD_SANDBOX_ROOTFS);

			if (g_mkdir_with_parents (config->pod->sandbox_workloads, CC_OCI_DIR_MODE)) {
				g_critical ("failed to create directory %s: %s",
					    config->pod->sandbox_workloads, strerror (errno));

				return -errno;
			}
		}
	} else if (g_strcmp0(annotation->key, CC_POD_OCID_SANDBOX_NAME) == 0) {
		if (config->pod->sandbox_name) {
			g_free(config->pod->sandbox_name);
		}
		config->pod->sandbox_name = g_strdup(annotation->value);
	}

	return 0;
}

/**
 * Free resources associated with \p CRI-O/ocid
 *
 * \param ocid \ref cc_ocid.
 *
 */
void
cc_pod_free (struct cc_pod *pod) {
	if (! pod) {
		return;
	}

	g_free_if_set (pod->sandbox_name);

	g_free (pod);
}

/*!
 * Start a container within a pod.
 *
 * \param config \ref cc_oci_config.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_pod_new_container (struct cc_oci_config *config)
{
	gboolean           ret = false;
	ssize_t            bytes;
	char               buffer[2] = { '\0' };
	g_autofree gchar  *timestamp = NULL;
	int                shim_err_fd = -1;
	int                shim_args_fd = -1;
	int                shim_socket_fd = -1;
	int                proxy_fd = -1;
	int                proxy_io_fd = -1;
	int                ioBase = -1;
	GSocketConnection *shim_socket_connection = NULL;
	GError            *error = NULL;

	if (! (config && config->pod && config->proxy)) {
		return false;
	}

	timestamp = cc_oci_get_iso8601_timestamp ();
	if (! timestamp) {
		goto out;
	}

	config->state.status = OCI_STATUS_CREATED;

	/* Connect and attach to the proxy first */
	if (! cc_proxy_connect (config->proxy)) {
		goto out;
	}

	if (! cc_proxy_attach (config->proxy, config->pod->sandbox_name)) {
		goto out;
	}

	/* Launch the shim child before the state file is created.
	 *
	 * Required since the state file must contain the workloads pid,
	 * and for our purposes the workload pid is the pid of the shim.
	 *
	 * The child blocks waiting for a write to shim_args_fd.
	 */
	if (! cc_shim_launch (config, &shim_err_fd, &shim_args_fd, &shim_socket_fd, true)) {
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
				&proxy_io_fd, &ioBase,
				config->oci.process.terminal)) {
		goto out;
	}

	bytes = write (shim_args_fd, &ioBase, sizeof (ioBase));
	if (bytes < 0) {
		g_critical ("failed to send proxy ioBase to shim child: %s",
			strerror (errno));
		goto out;
	}

	/* send proxy IO fd to cc-shim child */
	shim_socket_connection = cc_oci_socket_connection_from_fd(shim_socket_fd);
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

	/* Create the state file now that all information is
	 * available.
	 */
	g_debug ("recreating state file");

	ret = cc_oci_state_file_create (config, timestamp);
	if (! ret) {
		g_critical ("failed to recreate state file");
		goto out;
	}

	if (! cc_proxy_run_hyper_new_container (config,
						config->optarg_container_id, "")) {
		goto out;
	}

	/* We can now disconnect from the proxy (but the shim
	 * remains connected).
	 */
	ret = cc_proxy_disconnect (config->proxy);

	/* Finally, create the pid file. */
	if (config->pid_file) {
		ret = cc_oci_create_pidfile (config->pid_file,
				config->state.workload_pid);
		if (! ret) {
			goto out;
		}
	}
out:
	if (shim_err_fd != -1) close (shim_err_fd);
	if (shim_args_fd != -1) close (shim_args_fd);
	if (shim_socket_fd != -1) close (shim_socket_fd);

	return ret;
}
