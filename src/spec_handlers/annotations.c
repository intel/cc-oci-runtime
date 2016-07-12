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

#include "spec_handler.h"

static void
handle_annotation (GNode* root, struct clr_oci_config* config)
{
	struct oci_cfg_annotation *a = NULL;

	if (! (root && root->children)) {
		return;
	}

	if (! root->data) {
		return;
	}

	a = g_new0 (struct oci_cfg_annotation, 1);
	a->key = g_strdup ((gchar *)root->data);
	if (root->children->data) {
		a->value = g_strdup ((gchar *)root->children->data);
	}

	config->oci.annotations = g_slist_prepend
		(config->oci.annotations, a);
}

static bool
annotations_handle_section (GNode* root, struct clr_oci_config* config)
{
	g_node_children_foreach (root, G_TRAVERSE_ALL,
			(GNodeForeachFunc)handle_annotation, config);

	return true;
}

struct spec_handler annotations_spec_handler =
{
	.name = "annotations",
	.handle_section = annotations_handle_section
};
