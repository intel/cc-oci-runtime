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

#ifndef _CC_OCI_LOGGING_H
#define _CC_OCI_LOGGING_H

/** Mode for logfiles. */
#define CC_OCI_LOGFILE_MODE		0640

/** Options to pass to cc_oci_log_handler(). */
struct clr_log_options
{
    /* Full path to logfile to use. */
    char     *filename;

    /* Full path to global logfile to append to. */
    char     *global_logfile;

    /* If \c true, log in JSON, else ASCII. */
    gboolean  use_json;
};

gboolean cc_oci_log_init (const struct clr_log_options *options);
void cc_oci_log_free (struct clr_log_options *options);

#endif /* _CC_OCI_LOGGING_H */
