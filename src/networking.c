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

/** \file
 *
 * Networking routines, used to setup networking (currently docker specific)
 *
 */

#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if_arp.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "oci.h"
#include "util.h"

#define TUNDEV "/dev/net/tun"

/* Using unqualified path to enable it to be picked
 * up from the users enviornment. May be overidden
 * at build time with fully qualified path
 */
#ifndef CC_OCI_IPROUTE2_BIN
#define CC_OCI_IPROUTE2_BIN "ip"
#endif


/*!
 * Request to create a named tap interface
 *
 * \param tap \c tap interface name to create
 *
 * \return \c true on success, else \c false.
 */
static gboolean
cc_oci_tap_create(const gchar *tap) {
	struct ifreq ifr;
	int fd = -1;
	gboolean ret = false;

	if (tap == NULL) {
		g_critical("invalid tap interface");
		goto out;
	}

	fd = open(TUNDEV, O_RDWR);
	if (fd < 0) {
		g_critical("Failed to open [%s] [%s]", TUNDEV, strerror(errno));
		goto out;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP;
	g_strlcpy(ifr.ifr_name, tap, IFNAMSIZ);

	if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
		g_critical("Failed to create tap [%s] [%s]",
			tap, strerror(errno));
		goto out;
	}

	if (ioctl(fd, TUNSETPERSIST, 1) < 0) {
		g_critical("failed to TUNSETPERSIST [%s] [%s]",
			tap, strerror(errno));
		goto out;
	}

	ret = true;
out:
	if (fd != -1) {
		close(fd);
	}

	return ret;
}


/*!
 * Temporary function to invoke netlink commands
 * using the iproute2 binary. This will be replaced
 * by a netlink based implementation
 *
 * \param cmd_line \c iproute2 command to invoke
 *
 * \return \c true on success, else \c false.
 */
static gboolean
cc_oci_netlink_run(const gchar *cmd_line) {
	gint exit_status = -1;
	gboolean ret = false;
	gchar iproute2_cmd[1024];
	GError *error = NULL;

	if (cmd_line == NULL){
		g_critical("invalid netlink command");
		return false;
	}

	g_snprintf(iproute2_cmd, sizeof(iproute2_cmd),
		"%s %s", CC_OCI_IPROUTE2_BIN, cmd_line);

	ret = g_spawn_command_line_sync(iproute2_cmd,
				NULL,
				NULL,
				&exit_status,
				&error);

	if (!ret) {
		g_critical("failed to spawn [%s]: %s",
				iproute2_cmd, error->message);
		if (error) {
			g_error_free(error);
		}
		return false;
	}

	if (exit_status) {
		g_critical("netlink failure [%s] [%d]", iproute2_cmd, exit_status);
		return false;
	}

	return true;
}

/*!
 * Request to create the networking framework
 * that will be used to connect the specified
 * container network (veth) to the VM
 *
 * The container may be associated with multiple
 * networks and function has to be invoked for
 * each of those networks
 *
 * Once the OCI spec supports the creation of
 * VM compatible tap interfaces in the network
 * plugin, this setup will not be required
 *
 * \param config \ref cc_oci_config.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_network_create(const struct cc_oci_config *const config) {
	gchar cmd_line[1024];

	if (config == NULL){
		g_critical("invalid network configuration");
		return false;
	}

	/* No networking enabled */
	if (config->net.ifname == NULL) {
		return true;
	}

	if (!cc_oci_tap_create(config->net.tap_device)) {
		return false;
	}

	/* Place holder till we create these using appropriate
	* netlink commands
	*/

#define NETLINK_CREATE_BRIDGE "link add name %s type bridge"
	g_snprintf(cmd_line, sizeof(cmd_line),
		NETLINK_CREATE_BRIDGE,
		config->net.bridge);

	if (!cc_oci_netlink_run(cmd_line)) {
		return false;
	}

#define NETLINK_SET_MAC	"link set dev %s address %s"
	/* TODO: Derive non conflicting mac from interface */
	g_snprintf(cmd_line, sizeof(cmd_line),
		NETLINK_SET_MAC,
		config->net.ifname,
		"02:00:CA:FE:00:01");

	if (!cc_oci_netlink_run(cmd_line)) {
		return false;
	}

#define NETLINK_ADD_LINK_BR "link set dev %s master %s"
	g_snprintf(cmd_line, sizeof(cmd_line),
		NETLINK_ADD_LINK_BR,
		config->net.ifname,
		config->net.bridge);

	if (!cc_oci_netlink_run(cmd_line)) {
		return false;
	}

	g_snprintf(cmd_line, sizeof(cmd_line),
		NETLINK_ADD_LINK_BR,
		config->net.tap_device,
		config->net.bridge);

	if (!cc_oci_netlink_run(cmd_line)) {
		return false;
	}

#define NETLINK_LINK_EN	"link set dev %s up"
	g_snprintf(cmd_line, sizeof(cmd_line),
		NETLINK_LINK_EN,
		config->net.tap_device);

	if (!cc_oci_netlink_run(cmd_line)) {
		return false;
	}

	g_snprintf(cmd_line, sizeof(cmd_line),
		NETLINK_LINK_EN,
		config->net.ifname);

	if (!cc_oci_netlink_run(cmd_line)) {
		return false;
	}

	g_snprintf(cmd_line, sizeof(cmd_line),
		NETLINK_LINK_EN,
		config->net.bridge);

	if (!cc_oci_netlink_run(cmd_line)) {
		return false;
	}

	return true;
}

/*!
 * Obtain the string representation of the inet address
 *
 * \param family \c inetfamily
 * \param sin_addr \c inet address
 *
 * \return \c string containing IP address, else \c ""
 */
static gchar *
get_address(const gint family, const void *const sin_addr)
{
	gchar addrBuf[INET6_ADDRSTRLEN];

	if (!sin_addr) {
		return g_strdup("");
	}

	if (!inet_ntop(family, sin_addr, addrBuf, sizeof(addrBuf))) {
		g_critical("inet_ntop() failed with errno =  %d %s\n",
			errno, strerror(errno));
		return g_strdup("");
	}

	g_debug("IP := [%s]", addrBuf);
	return g_strdup(addrBuf);
}

/*!
 * Obtain the string representation of the mac address
 * of an interface
 *
 * \param ifname \c inetface name
 *
 * \return \c string containing MAC address, else \c ""
 */
static gchar *
get_mac_address(const gchar *const ifname)
{
	struct ifreq ifr;
	int fd = -1;
	gchar *macaddr;
	gchar *data;

	if (ifname == NULL) {
		g_critical("NULL interface name");
		return g_strdup("");
	}

	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (fd < 0) {
		g_critical("socket() failed with errno =  %d %s\n",
			errno, strerror(errno));
		return g_strdup("");
	}

	memset(&ifr, 0, sizeof(ifr));
	g_strlcpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
		g_critical("ioctl() failed with errno =  %d %s\n",
			errno, strerror(errno));
		macaddr = g_strdup("");
		goto out;
	}

	if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
		g_critical("invalid interface  %s\n", ifname);
		macaddr = g_strdup("");
		goto out;
	}

	data = ifr.ifr_hwaddr.sa_data;
	macaddr = g_strdup_printf("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
				(unsigned char)data[0],
				(unsigned char)data[1],
				(unsigned char)data[2],
				(unsigned char)data[3],
				(unsigned char)data[4],
				(unsigned char)data[5]);

out:
	close(fd);
	return macaddr;
}



/*!
 * Obtain the default gateway in a interface
 * Temporary: Will be replaced by netlink based implementation
 *
 * \param ifname \c interface name
 *
 * \return \c string containing default gw on that interface, else \c ""
 */
static gchar *
get_default_gw(const gchar *const ifname)
{
	gint exit_status = -1;
	gboolean ret;
	gchar iproute2_cmd[1024];
	gchar *output = NULL;
	gchar **output_tokens = NULL;
	gint token_count;
	gchar *gw = NULL;
	struct sockaddr_in sa;

	if (ifname == NULL) {
		g_critical("NULL interface name");
		return false;
	}

#define NETLINK_GET_DEFAULT_GW CC_OCI_IPROUTE2_BIN " -4 route list type unicast dev %s exact 0/0"

	g_snprintf(iproute2_cmd, sizeof(iproute2_cmd),
		NETLINK_GET_DEFAULT_GW, ifname);

	ret = g_spawn_command_line_sync(iproute2_cmd,
				&output,
				NULL,
				&exit_status,
				NULL);

	if (!ret) {
		g_critical("failed to spawn [%s]", iproute2_cmd);
		goto out;
	}

	if (exit_status) {
		g_critical("netlink failure [%s] [%d]", iproute2_cmd, exit_status);
		goto out;
	}

	if (output == NULL) {
		goto out;
	}

	output_tokens = g_strsplit(output, " ", -1);

	for (token_count=0; output_tokens && output_tokens[token_count];) {
	     token_count++;
	}

	if (token_count < 3) {
		g_critical("unable to discover gateway [%s]", output);
		goto out;
	}

	if (inet_pton(AF_INET, output_tokens[2], &(sa.sin_addr)) != 1) {
		g_critical("invalid gateway [%s] [%s]", output, output_tokens[2]);
		goto out;
	}

	gw = g_strdup(output_tokens[2]);

out:
	if (output_tokens != NULL) {
		g_strfreev(output_tokens);
	}
	if (output != NULL) {
		g_free(output);
	}
	if (gw == NULL) {
		return g_strdup("");
	}
	return gw;
}

/*!
 * Obtain the network configuration of the container
 * Currently done by by scanned the namespace
 * Ideally the OCI spec should be modified such that
 * these parameters are sent to the runtime
 *
 * The current implementation attempts to find the
 * first interface with a valid IPv4 address.
 *
 * TODO: Support multiple interfaces and IPv6
 *
 * \param[in,out] config \ref cc_oci_config.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_network_discover(struct cc_oci_config *const config)
{
	struct ifaddrs *ifa = NULL;
	struct ifaddrs *ifaddrs = NULL;
	gint family;
	gchar *ifname;

	if (!config) {
		return false;
	}

	if (getifaddrs(&ifaddrs) == -1) {
		g_critical("getifaddrs() failed with errno =  %d %s\n",
			errno, strerror(errno));
		return false;
	}

	g_debug("Discovering container interfaces");

	/* For now pick the first interface with a valid IP address */
	for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr) {
			continue;
		}

		g_debug("Interface := [%s]", ifa->ifa_name);

		family = ifa->ifa_addr->sa_family;
		if (family != AF_INET) {
			continue;
		}

		if (!g_strcmp0(ifa->ifa_name, "lo")) {
			continue;
		}

		ifname = ifa->ifa_name;
		config->net.ifname = g_strdup(ifname);
		config->net.mac_address = get_mac_address(ifname);
		config->net.tap_device = g_strdup_printf("c%s", ifname);
		config->net.bridge = g_strdup_printf("b%s", ifname);
		config->net.ip_address = get_address(family,
			&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr);
		config->net.subnet_mask = get_address(family,
			&((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr);
		config->net.gateway = get_default_gw(ifname);
		break;
	}

	freeifaddrs(ifaddrs);

	if (config->oci.hostname) {
		config->net.hostname = g_strdup(config->oci.hostname);
	} else {
		config->net.hostname = g_strdup("");
	}

	if (config->net.gateway == NULL) {
		config->net.gateway = g_strdup("");
	}

	/* TODO: Need to see if this needed, does resolv.conf handle this */
	config->net.dns_ip1 = g_strdup("");
	config->net.dns_ip2 = g_strdup("");

	if (!config->net.ifname) {
		g_debug("No container networks discovered, networking disabled");
	}

	return true;
}
