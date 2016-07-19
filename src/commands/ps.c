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

#include "command.h"
#include "state.h"


static gboolean
handler_ps (const struct subcommand *sub,
		struct cc_oci_config *config,
		int argc, char *argv[])
{
	// default ps options
	gchar *ps_options = "-ef";

	g_assert (sub);
	g_assert (config);

	if ((argc && ((!g_strcmp0 (argv[0], "--help")) ||
		     (!g_strcmp0 (argv[0], "-h")))) ||
		     (!argc)) {
		g_print ("Usage: %s [command options] <container-id> <ps options>\n",
		    sub->name);

		return argc ? true : false;
	}

	config->optarg_container_id = argv[0];

	if (argc > 1) {
		ps_options = argv[1];
	} else {

	}

	if (! cc_oci_state_file_exists(config)) {
		g_warning ("state file does not exist for container %s",
				config->optarg_container_id);
		return false;
	}

	//FIXME: implement ps, run ps inside the VM ?

	return true;
}

struct subcommand command_ps =
{
	.name        = "ps",
	.handler     = handler_ps,
	.description = "display the processes running inside a container",
};
