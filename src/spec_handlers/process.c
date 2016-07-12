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

#include <stdio.h>
#include <stdlib.h>

#include "spec_handler.h"
#include "util.h"

static void
handle_user_section (GNode *root, struct clr_oci_config *config)
{
	if (! root) {
		return;
	}

	if (g_strcmp0(root->data, "uid") == 0) {
		config->oci.process.user.uid = (uid_t)atoi (root->children->data);
	}
	if (g_strcmp0(root->data, "gid") == 0) {
		config->oci.process.user.gid = (gid_t)atoi (root->children->data);
	}
}

static void
handle_process_section(GNode* root, struct clr_oci_config* config) {
	if (! (root && root->children)) {
		return;
	}
	if (g_strcmp0(root->data, "cwd") == 0) {
		if (snprintf(config->oci.process.cwd,
		    sizeof(config->oci.process.cwd),
		    "%s", (char*)root->children->data) < 0) {
			g_critical("failed to copy process cwd");
		}
	} else if(g_strcmp0(root->data, "args") == 0) {
		config->oci.process.args = node_to_strv(root);
	} else if(g_strcmp0(root->data, "env") == 0) {
		config->oci.process.env = node_to_strv(root);
	} else if(g_strcmp0(root->data, "terminal") == 0) {
		config->oci.process.terminal =
			!g_strcmp0 ((gchar *)root->children->data, "true")
			? true : false;
	} else if(g_strcmp0(root->data, "user") == 0) {
		g_node_children_foreach (root, G_TRAVERSE_ALL,
				(GNodeForeachFunc)handle_user_section, config);
	}
}

static bool
process_handle_section(GNode* root, struct clr_oci_config* config) {
	g_node_children_foreach(root, G_TRAVERSE_ALL,
		(GNodeForeachFunc)handle_process_section, config);

	return true;
}

struct spec_handler process_spec_handler = {
	.name = "process",
	.handle_section = process_handle_section
};
