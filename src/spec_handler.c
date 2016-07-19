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

#include "spec_handler.h"
#include "util.h"
#include "json.h"

extern struct spec_handler annotations_spec_handler;
extern struct spec_handler hooks_spec_handler;
extern struct spec_handler mounts_spec_handler;
extern struct spec_handler platform_spec_handler;
extern struct spec_handler process_spec_handler;
extern struct spec_handler root_spec_handler;
extern struct spec_handler vm_spec_handler;
extern struct spec_handler linux_spec_handler;

static struct spec_handler* stop_spec_handlers[] = {
	&hooks_spec_handler,

	/* terminator */
	NULL
};

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

// FIXME: document
void
process_config_stop (GNode* root, struct cc_oci_config* config) {
	/* looking for right spec handler */
	for (struct spec_handler** i=stop_spec_handlers; (*i); ++i) {
		if (g_strcmp0((*i)->name, root->data) == 0) {
			/* run spec handler */
			if (! (*i)->handle_section(root, config)) {
				g_critical("failed spec handler: %s", (*i)->name);
			}
			return;
		}
	}
}

// FIXME: document
void
process_config_start (GNode* root, struct cc_oci_config* config) {
	if (!(root && root->data)) {
		return;
	}

	if (g_strcmp0 (root->data, "ociVersion") == 0) {
		config->oci.oci_version = g_strdup (root->children->data);
	}

	if (g_strcmp0 (root->data, "hostname") == 0) {
		config->oci.hostname = g_strdup (root->children->data);
	}

	/* looking for right spec handler */
	for (struct spec_handler** i=start_spec_handlers; (*i); ++i) {
		if (g_strcmp0((*i)->name, root->data) == 0) {
			/* run spec handler */
			if (! (*i)->handle_section(root, config)) {
				g_critical("failed spec handler: %s", (*i)->name);
			}
			return;
		}
	}
}

/*!
 * If the virtual machine attribute ("vm") in config is NULL,
 * this function will create create it using the json from
 * SYSCONFDIR/CC_OCI_VM_CONFIG
 * or fallback default DEFAULTSDIR/CC_OCI_VM_CONFIG
 *
 * \param[in,out] config cc_oci_config struct
 *
 * \return \c true if can get vm spec data, else \c false.
 */
gboolean
get_spec_vm_from_cfg_file (struct cc_oci_config* config)
{
	bool result= true;
	GNode* vm_config = NULL;
	GNode* vm_node= NULL;
	gchar* sys_json_file = NULL;

	if (config->vm) {
		/* If vm spec data exist, do nothing */
		goto out;
	}
	sys_json_file = g_build_path ("/", SYSCONFDIR,
		CC_OCI_VM_CONFIG, NULL);
	if (! g_file_test (sys_json_file, G_FILE_TEST_EXISTS)) {
		g_free_if_set (sys_json_file);
		sys_json_file = g_build_path ("/", DEFAULTSDIR,
		CC_OCI_VM_CONFIG, NULL);
	}
	g_debug ("Reading VM configuration from %s",
		sys_json_file);
	if (! cc_oci_json_parse(&vm_config, sys_json_file)) {
		result = false;
		goto out;
	}

	vm_node = g_node_first_child(vm_config);
	while (vm_node) {
		if (g_strcmp0(vm_node->data, vm_spec_handler.name) == 0) {
			break;
		}
		vm_node = g_node_next_sibling(vm_node);
	}
	vm_spec_handler.handle_section(vm_node, config);
	if (! config->vm) {
		g_critical ("VM json node not found");
		result = false;
	}
out:
	g_free_if_set (sys_json_file);
	g_free_node (vm_config);
	return result;
}
