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

#ifndef _CC_OCI_PROXY_H
#define _CC_OCI_PROXY_H

#include <stdbool.h>
#include <gio/gio.h>
#include "oci.h"

gboolean cc_proxy_connect (struct cc_proxy *proxy);
gboolean cc_proxy_disconnect (struct cc_proxy *proxy);
gboolean cc_proxy_wait_until_ready (struct cc_oci_config *config);
gboolean cc_proxy_hyper_pod_create (struct cc_oci_config *config);
gboolean cc_proxy_cmd_bye (struct cc_proxy *proxy);
void cc_proxy_free (struct cc_proxy *proxy);

#endif /* _CC_OCI_PROXY_H */
