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

#ifndef _CLR_OCI_NETWORK_H
#define _CLR_OCI_NETWORK_H

gboolean clr_oci_vm_pause (const gchar *socket_path, GPid pid);
gboolean clr_oci_vm_resume (const gchar *socket_path, GPid pid);
gboolean clr_oci_vm_shutdown (const gchar *socket_path, GPid pid);

#endif /* _CLR_OCI_NETWORK_H */
