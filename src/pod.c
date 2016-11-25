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

/* CRI-O/ocid namespaces */
#define CC_POD_OCID_NAMESPACE "ocid/"
#define CC_POD_OCID_NAMESPACE_SIZE 5

#define CC_POD_OCID_CONTAINER_TYPE "ocid/container_type"
#define CC_POD_OCID_SANDBOX        "sandbox"
#define CC_POD_OCID_CONTAINER      "container"

#define CC_POD_OCID_SANDBOX_NAME "ocid/sandbox_name"

#include <errno.h>
#include <string.h>

#include "pod.h"

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
