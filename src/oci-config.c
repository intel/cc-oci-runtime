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

#include <string.h>
#include <stdbool.h>

#include <glib.h>
#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

#include "util.h"
#include "mount.h"
#include "annotation.h"
#include "namespace.h"
#include "oci.h"
#include "semver.h"
#include "oci-config.h"

/*!
 * Free all resources associated with \p h hook object.
 *
 * \param h \ref oci_cfg_hook.
 */
void
cc_oci_hook_free (struct oci_cfg_hook *h) {
	if (h) {
		if (h->args) {
			g_strfreev(h->args);
		}
		if (h->env) {
			g_strfreev(h->env);
		}
		g_free(h);
	}
}

/*!
 * Perform checks on the specified config.
 *
 * \param config \ref cc_oci_config.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_config_check (const struct cc_oci_config *config)
{
	gint  ret;

	if (! config) {
		return false;
	}

	if (! config->oci.oci_version) {
		g_critical ("no OCI version specified");
		return false;
	}

	ret = cc_oci_semver_cmp (CC_OCI_SUPPORTED_SPEC_VERSION,
			config->oci.oci_version);

	if (ret < 0) {
		g_critical ("cannot handle config version %s "
				"(supported version is %s)",
				config->oci.oci_version,
				CC_OCI_SUPPORTED_SPEC_VERSION);
		return false;
	}

	return true;
}

/*!
 * Get the path of \ref CC_OCI_CONFIG_FILE.
 *
 * \param bundle_path Full path to containers bundle path.
 *
 * \return Newly-allocated path string on success, else \c NULL.
 */
gchar *
cc_oci_config_file_path (const gchar *bundle_path)
{
	if (! bundle_path) {
		return NULL;
	}

	return cc_oci_get_bundlepath_file (bundle_path,
			CC_OCI_CONFIG_FILE);
}

/*!
 * Free all resources associated with the static \p config object.
 *
 * \param config \ref cc_oci_config.
 */
void
cc_oci_config_free (struct cc_oci_config *config)
{
	if (! config) {
		return;
	}

	g_free_if_set (config->oci.oci_version);
	g_free_if_set (config->oci.hostname);
	g_free_if_set (config->console);
	g_free_if_set (config->bundle_path);
	g_free_if_set (config->root_dir);
	g_free_if_set (config->pid_file);

	if (config->vm) {
		g_free_if_set (config->vm->kernel_params);
		g_free (config->vm);
	}

	if (config->oci.process.args) {
		g_strfreev (config->oci.process.args);
	}

	if (config->oci.process.env) {
		g_strfreev (config->oci.process.env);
	}

	cc_oci_mounts_free_all (config->oci.mounts);
	cc_oci_annotations_free_all (config->oci.annotations);

	g_free_if_set (config->oci.platform.os);
	g_free_if_set (config->oci.platform.arch);

	if (config->oci.hooks.prestart) {
		g_slist_free_full(config->oci.hooks.prestart,
                (GDestroyNotify)cc_oci_hook_free);
	}
	if (config->oci.hooks.poststart) {
		g_slist_free_full(config->oci.hooks.poststart,
                (GDestroyNotify)cc_oci_hook_free);
	}
	if (config->oci.hooks.poststop) {
		g_slist_free_full(config->oci.hooks.poststop,
                (GDestroyNotify)cc_oci_hook_free);
	}
	if (config->oci.oci_linux.namespaces) {
		g_slist_free_full(config->oci.oci_linux.namespaces,
                (GDestroyNotify)cc_oci_ns_free);
	}

	g_free_if_set (config->net.hostname);
	g_free_if_set (config->net.gateway);
	g_free_if_set (config->net.mac_address);
	g_free_if_set (config->net.ip_address);
	g_free_if_set (config->net.subnet_mask);
	g_free_if_set (config->net.ipv6_address);
	g_free_if_set (config->net.ifname);
	g_free_if_set (config->net.bridge);
	g_free_if_set (config->net.tap_device);
	g_free_if_set (config->net.dns_ip1);
	g_free_if_set (config->net.dns_ip2);
}
