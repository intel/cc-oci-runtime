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

#ifndef _CLR_OCI_STATE_H
#define _CLR_OCI_STATE_H

gboolean clr_oci_state_file_get (struct clr_oci_config *config);
struct oci_state *clr_oci_state_file_read (const char *file);
void clr_oci_state_free (struct oci_state *state);
gboolean clr_oci_state_file_create (struct clr_oci_config *config,
		const char *created_timestamp);
gboolean clr_oci_state_file_delete (const struct clr_oci_config *config);
gboolean clr_oci_state_file_exists (struct clr_oci_config *config);
const char *clr_oci_status_to_str (enum oci_status status);
enum oci_status clr_oci_str_to_status (const char *str);
int clr_oci_status_length (void);

#endif /* _CLR_OCI_STATE_H */
