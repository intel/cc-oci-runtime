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

#include "command.h"

static gboolean
handler_attach (const struct subcommand *sub,
		struct clr_oci_config *config,
		int argc, char *argv[])
{
	struct oci_state  *state = NULL;
	gchar             *config_file = NULL;
	gboolean           ret;

	g_assert (sub);
	g_assert (config);

	if (handle_default_usage (argc, argv, sub->name, &ret)) {
		return ret;
	}

	config->optarg_container_id = argv[0];

	ret = clr_oci_get_config_and_state(&config_file, config, &state);
	if (! ret) {
		goto out;
	}

	ret = clr_oci_attach(config, state);
	if (! ret) {
		goto out;
	}

	ret = true;

out:
	g_free_if_set (config_file);
	clr_oci_state_free (state);
	return ret;
}

struct subcommand command_attach =
{
	.name    = "attach",
	.handler = handler_attach,
	.description = "attach to a container",
};
