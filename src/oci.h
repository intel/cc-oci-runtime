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

/**
 * \file
 *
 * Open Container Initiative (OCI) defines and types.
 *
 * \see https://www.opencontainers.org/.
 */

#ifndef _CC_OCI_H
#define _CC_OCI_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#include <mntent.h>
#include <sys/mount.h>

/* Required for CLONE_* */
#define _GNU_SOURCE
#include <sched.h>
#include <linux/sched.h>

#include <glib.h>

#ifndef CLONE_NEWCGROUP
#define CLONE_NEWCGROUP 0x02000000
#endif

/** Version of the https://github.com/opencontainers/specs we support. */
#define CC_OCI_SUPPORTED_SPEC_VERSION	"1.0.0-rc1"

/** Name of OCI configuration file. */
#define CC_OCI_CONFIG_FILE		"config.json"

/** Name of hypervisor socket used to control an already running VM */
#define CC_OCI_HYPERVISOR_SOCKET	"hypervisor.sock"

/** Name of hypervisor socket used to determine if VM is running */
#define CC_OCI_PROCESS_SOCKET		"process.sock"

/** Name of hypervisor socket used as a console device. */
#define CC_OCI_CONSOLE_SOCKET		"console.sock"

/** File generated below \ref CC_OCI_RUNTIME_DIR_PREFIX at runtime that
 * contains metadata about the running instance.
 */
#define CC_OCI_STATE_FILE		"state.json"

/** Directory below which container-specific directory will be created.
 */
#define CC_OCI_RUNTIME_DIR_PREFIX	"/run/opencontainer/containers"

/** Mode for \ref CC_OCI_WORKLOAD_FILE. */
#define CC_OCI_SCRIPT_MODE		0755

/** Mode for all created directories. */
#define CC_OCI_DIR_MODE                0750

/** Platform we expect \ref CC_OCI_CONFIG_FILE to specify. */
#define CC_OCI_EXPECTED_PLATFORM	"linux"

/** Architecture we expect \ref CC_OCI_CONFIG_FILE to specify. */
#define CC_OCI_EXPECTED_ARCHITECTURE	"amd64"

/** File that will be executed automatically on VM boot by
 * container-workload.service.
 */
#define CC_OCI_WORKLOAD_FILE		"/.containerexec"

/** Name of file containing environment variables that will be set
 * inside the VM.
 */
#define CC_OCI_ENV_FILE		"/.containerenv"

/** Shell to use for \ref CC_OCI_WORKLOAD_FILE. */
#define CC_OCI_WORKLOAD_SHELL		"/bin/sh"

/** Command to use to allow "exec" to connect to the container. */
#define CC_OCI_EXEC_CMD               "ssh"

/** File that contains vm spec configuration, used if vm node
 * in CC_OCI_CONFIG_FILE bundle file
*/
#define CC_OCI_VM_CONFIG "vm.json"

/* Path to the passwd formatted file. */
#define PASSWD_PATH "/etc/passwd"

/* Path to the stateless passwd file. */ 
#define STATELESS_PASSWD_PATH "/usr/share/defaults/etc/passwd"

/** Status of an OCI container. */
enum oci_status {
	OCI_STATUS_CREATED = 0,
	OCI_STATUS_RUNNING,
	OCI_STATUS_PAUSED,
	OCI_STATUS_STOPPED,
	OCI_STATUS_STOPPING,
	OCI_STATUS_INVALID = -1
};

enum oci_namespace {
	OCI_NS_PID     = CLONE_NEWPID,
	OCI_NS_NET     = CLONE_NEWNET,
	OCI_NS_MOUNT   = CLONE_NEWNS,
	OCI_NS_IPC     = CLONE_NEWIPC,
	OCI_NS_UTS     = CLONE_NEWUTS,
	OCI_NS_USER    = CLONE_NEWUSER,
	OCI_NS_CGROUP  = CLONE_NEWCGROUP,

	OCI_NS_INVALID = -1,
};

struct oci_cfg_platform {
	gchar  *os;
	gchar  *arch;
};

struct oci_cfg_root {
	/** Full path to chroot workload directory. */
	gchar     path[PATH_MAX];

	gboolean  read_only;
};

struct oci_cfg_user {
	uid_t    uid; /*!< User ID to run workload as. */
	gid_t    gid; /*!< Group ID to run workload as. */
	gid_t  **additionalGids; /*!< extra Group IDs to set workload as a member of */
};

struct oci_cfg_hook {
	gchar    path[PATH_MAX]; /*!< Hook command to run. */
	gchar  **args;           /*!< Arguments to command (argv[0] is the first argument). */
	gchar  **env;            /*!< List of environment variables to set. */

	/** \warning FIXME: hook timeout not in the official schema yet!?! */
	gint     timeout;
};

struct oci_cfg_hooks {
	GSList	*prestart;
	GSList	*poststart;
	GSList	*poststop;
};

struct oci_cfg_annotation {
	gchar  *key;
	gchar  *value;
};

struct oci_cfg_namespace {
	enum oci_namespace  type;
	gchar              *path;
};

/**
 * Representation of OCI process configuration.
 *
 * \see https://github.com/opencontainers/runtime-spec/blob/master/config.md#process-configuration
 *
 * \note Limitation: None of the Linux-specific elements (like
 * capabilities, security profiles and rlimits) are supported.
 */
struct oci_cfg_process {
	gchar              **args;

	/** Full path to working directory to run workload command in. */
	gchar                cwd[PATH_MAX];

	gchar              **env;

	/** Set to \c true if the container has an associated terminal. */
	gboolean             terminal;

	struct oci_cfg_user  user;
};

/**
 * Representation of OCI linux-specific configuration.
 *
 * \see
 * https://github.com/opencontainers/runtime-spec/blob/master/config-linux.md
 *
 * \note For now, we only care about namespaces.
 */
struct oci_cfg_linux {
	/** List of \ref oci_cfg_namespace namespaces */
	GSList          *namespaces;
};

/** Representation of the OCI runtime schema embodied by
 * \ref CC_OCI_CONFIG_FILE.
 *
 * \see https://github.com/opencontainers/runtime-spec/blob/master/schema/schema.json
 */
struct oci_cfg {
	gchar                       *oci_version;
	struct oci_cfg_hooks         hooks;
	gchar                       *hostname;

	/** List of \ref cc_oci_mount mounts.
	 *
	 * These are handled specially (due to mount flags),
	 * so there is no \c oci_cfg_mount type.
	 */
	GSList                      *mounts;

	/** List of \ref oci_cfg_annotation annotations.
	 *
	 * These are handled with a list for simplicity.
	 */
	GSList                      *annotations;

	struct oci_cfg_platform      platform;
	struct oci_cfg_root          root;
	struct oci_cfg_process       process;

	/* XXX: can't call it "linux" due to preprocessor clash :) */
	struct oci_cfg_linux         oci_linux;
};

/** clr-specific VM configuration data. */
struct cc_oci_vm_cfg {
	/** Full path to the hypervisor. */
	gchar hypervisor_path[PATH_MAX];

	/** Full path to Clear Containers disk image. */
	gchar image_path[PATH_MAX];

	/** Full path to kernel to use for VM. */
	gchar kernel_path[PATH_MAX];

	/** Full path to CC_OCI_WORKLOAD_FILE
	 * (which exists below "root_path").
	 */
	gchar workload_path[PATH_MAX];

	/** Kernel parameters (optional). */
	gchar *kernel_params;
};

/** cc-specific network configuration data. */
struct cc_oci_net_cfg {

	/** Network gateway (xxx.xxx.xxx.xxx). */
	gchar  *hostname;

	/** Network gateway (xxx.xxx.xxx.xxx). */
	gchar  *gateway;

	/** TODO: Do not limit number of DNS servers */

	/** DNS IP (xxx.xxx.xxx.xxx). */
	gchar  *dns_ip1;

	/** DNS IP (xxx.xxx.xxx.xxx). */
	gchar  *dns_ip2;

	/** Network interfaces. */
	GSList  *interfaces;

	/** TODO: Add support for routes */
};


/** cc-specific network interface configuration data. */
struct cc_oci_net_if_cfg {

	/** MAC address with colon separators (xx:xx:xx:xx:xx:xx). */
	gchar  *mac_address;

	/** Name of network interface (veth) within the namespace
	 * This should also be the name of the interface within the VM */
	gchar  *ifname;

	/** Name of network bridge using to connect if to tap_device. */
	gchar  *bridge;

	/** Name of the QEMU tap device */
	gchar  *tap_device;

	/** List of IPv4 addresses on the interface */
	GSList  *ipv4_addrs;

	/** List of IPv6 addresses on the interface */
	GSList  *ipv6_addrs;

	/** TODO: Add support for routes */
};

/** cc-specific IPv4 configuration data. */
struct cc_oci_net_ipv4_cfg {
	/** IPv4 address (xxx.xxx.xxx.xxx). */
	gchar  *ip_address;

	/** IPv4 subnet mask (xxx.xxx.xxx.xxx). */
	gchar  *subnet_mask;
};

/** cc-specific IPv6 configuration data. */
struct cc_oci_net_ipv6_cfg {
	/** IPv6 address (x:y::a:z). */
	gchar  *ipv6_address;
	/** IPv6 prefix */
	gchar  *ipv6_prefix;
};

/**
 * Generic type that maps an integer value
 * to a human-readable string.
 */
struct cc_oci_map {
	int           num;
	const gchar  *name;
};

/** OCI State, read from \ref CC_OCI_STATE_FILE.
 *
 * \see https://github.com/opencontainers/runtime-spec/blob/master/runtime.md#state
 *
 * Note that additional fields are added to allow commands other than
 * "start" to perform their functions.
 */
struct oci_state {
	gchar           *oci_version;
	gchar           *id; /*!< Container id. */

	/** Process ID of VM. */
	GPid             pid;

	gchar           *bundle_path;
	gchar           *comms_path;

	/** path to the process socket, used to determine when the VM
	 * has shut down.
	 */
	gchar           *procsock_path;
	enum oci_status  status; /*!< OCI status of container. */
	gchar           *create_time; /*!< ISO 8601 timestamp. */

	/** List of \ref cc_oci_mount mounts.
	 *
	 * These are handled specially (due to mount flags),
	 * so there is no \c oci_cfg_mount type.
	 */
	GSList          *mounts;

        /** List of \ref oci_cfg_annotation annotations.
         *
         */
        GSList          *annotations;

	/* See member of same name in \ref cc_oci_config. */
	gchar           *console;

	/* See member of same name in \ref cc_oci_config. */
	gboolean         use_socket_console;

	struct cc_oci_vm_cfg *vm;
};

/** clr-specific state fields. */
struct cc_oci_container_state {
	/** Full path to generated state file. */
	gchar state_file_path[PATH_MAX];

	/** Full path to container-specific directory below
	 * \ref CC_OCI_RUNTIME_DIR_PREFIX (or below the modified root
	 * specified in \ref cc_oci_config).
	 */
	gchar runtime_path[PATH_MAX];

	/** Full path to socket used to control the hypervisor
	 * below \ref CC_OCI_RUNTIME_DIR_PREFIX (or below the modified
	 * root specified in \ref cc_oci_config).
	 */
	gchar comms_path[PATH_MAX];

	/** Full path to socket used to determine when the hypervisor
	 * has been shut down.
	 * Created below \ref CC_OCI_RUNTIME_DIR_PREFIX
	 * (or below the modified root specified in \ref cc_oci_config).
	 */
	gchar procsock_path[PATH_MAX];

	/* Process ID of hypervisor. */
	GPid workload_pid;

	/** OCI status of container. */
	enum oci_status status;
};

/** clr-specific mount details. */
struct cc_oci_mount {
	/** Flags to pass to \c mount(2). */
	unsigned long  flags;

	struct mntent  mnt;

	/** Full path to mnt_dir directory
	 * (root_dir + '/' + mnt.mnt_dir).
	 */
	gchar          dest[PATH_MAX];

	/** \c true if mount should not be honoured. */
	gboolean       ignore_mount;

	/** Full path to first parent directory created to mount dest
	 * this path will be deleted after umount dest in order to
	 * left the rootfs in its original state.
	 * NULL if no directory was created to mount dest
	 */
	gchar          *directory_created;
};

/** The main object holding all configuration data.
 *
 * \note The main user of this object is "start" - other commands
 * generally use \ref oci_state and then partially "fill in" the
 * appropriate fields in a \ref cc_oci_config object depending on what
 * they are trying to accomplish.
 */
struct cc_oci_config {
	/** Official OCI configuration parameters. */
	struct oci_cfg                  oci;

	/** VM configuration
	 * (which should eventually be encoded in OCI configuration).
	 */
	struct cc_oci_vm_cfg           *vm;

	/** Network configuration. */
	struct cc_oci_net_cfg           net;

	/** Container-specific state. */
	struct cc_oci_container_state  state;

	/** Path to directory containing OCI bundle to run. */
	gchar *bundle_path;

	/** Path to file to store Process ID in. */
	gchar *pid_file;

	/** Path to device to use for I/O. */
	gchar *console;

	/** If \c true, \ref console will be a socket rather than a pty
	 * device.
	 */
	gboolean use_socket_console;

	/** If set, use an alternative root directory to the default
	 * CC_OCI_RUNTIME_DIR_PREFIX.
	 */
	gchar *root_dir;

	/* optarg value (do not free!) */
	const gchar *optarg_container_id;

	/** If \c true, don't start the VM */
	gboolean dry_run_mode;

	/** If \c true, don't wait for hypervisor process to finish. */
	gboolean detached_mode;
};

gboolean cc_oci_attach(struct cc_oci_config *config,
		struct oci_state *state);
gchar *cc_oci_config_file_path (const gchar *bundle_path);
gboolean cc_oci_create (struct cc_oci_config *config);
gboolean cc_oci_start (struct cc_oci_config *config,
		struct oci_state *state);
gboolean cc_oci_run (struct cc_oci_config *config);
void cc_oci_config_free (struct cc_oci_config *config);
gchar *cc_oci_get_bundlepath_file (const gchar *bundle_path,
		const gchar *file);
gboolean cc_oci_get_config_and_state (gchar **config_file,
		struct cc_oci_config *config,
		struct oci_state **state);
void cc_oci_state_free (struct oci_state *state);
gboolean cc_oci_stop (struct cc_oci_config *config,
        struct oci_state *state);
gboolean cc_oci_toggle (struct cc_oci_config *config,
		struct oci_state *state, gboolean pause);
gboolean cc_oci_exec (struct cc_oci_config *config,
		struct oci_state *state,
		int argc, char *const args[]);
gboolean cc_oci_list (struct cc_oci_config *config,
		const gchar *format, gboolean show_all);
gboolean cc_oci_delete (struct cc_oci_config *config,
		struct oci_state *state);
gboolean cc_oci_kill (struct cc_oci_config *config,
		struct oci_state *state, int signum);

gboolean cc_oci_config_update (struct cc_oci_config *config,
		struct oci_state *state);
gboolean
cc_oci_create_container_networking_workload (struct cc_oci_config *config);

#endif /* _CC_OCI_H */
