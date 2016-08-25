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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *	Copyright (c) 1983, 1993
 *		The Regents of the University of California.  All rights reserved.
 *
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions
 *	are met:
 *	1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 *	2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 *	4. Neither the name of the University nor the names of its contributors
 *	may be used to endorse or promote products derived from this software
 *	without specific prior written permission.
 *
 *	THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 *	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *	ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 *	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *	OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *	OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *	SUCH DAMAGE.
 *
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
 * up from the users environment. May be overridden
 * at build time with fully qualified path
 */
#ifndef CC_OCI_IPROUTE2_BIN
#define CC_OCI_IPROUTE2_BIN "ip"
#endif

/*!
 * Free the specified \ref cc_oci_net_ipv4_cfg
 *
 * \param if_cfg \ref cc_oci_net_ipv4_cfg
 */
static void
cc_oci_net_ipv4_free (struct cc_oci_net_ipv4_cfg *ipv4_cfg)
{
	if (!ipv4_cfg) {
		return;
	}

	g_free_if_set (ipv4_cfg->ip_address);
	g_free_if_set (ipv4_cfg->subnet_mask);
	g_free (ipv4_cfg);
}

/*!
 * Free the specified \ref cc_oci_net_ipv6_cfg
 *
 * \param if_cfg \ref cc_oci_net_ipv6_cfg
 */
static void
cc_oci_net_ipv6_free (struct cc_oci_net_ipv6_cfg *ipv6_cfg)
{
	if (!ipv6_cfg) {
		return;
	}

	g_free_if_set (ipv6_cfg->ipv6_address);
	g_free (ipv6_cfg);
}

/*!
 * Free the specified \ref cc_oci_net_if_cfg
 *
 * \param if_cfg \ref cc_oci_net_if_cfg
 */
void
cc_oci_net_interface_free (struct cc_oci_net_if_cfg *if_cfg)
{
	if (!if_cfg) {
		return;
	}

	g_free_if_set (if_cfg->mac_address);
	g_free_if_set (if_cfg->ifname);
	g_free_if_set (if_cfg->bridge);
	g_free_if_set (if_cfg->tap_device);

	if (if_cfg->ipv4_addrs) {
		g_slist_free_full(if_cfg->ipv4_addrs,
                (GDestroyNotify)cc_oci_net_ipv4_free);
	}

	if (if_cfg->ipv6_addrs) {
		g_slist_free_full(if_cfg->ipv6_addrs,
                (GDestroyNotify)cc_oci_net_ipv6_free);
	}

	g_free (if_cfg);
}


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
	struct cc_oci_net_if_cfg *if_cfg = NULL;
	gchar cmd_line[1024];
	guint index = 0;

	if (config == NULL) {
		return false;
	}

	for (index=0; index<g_slist_length(config->net.interfaces); index++) {
		if_cfg = (struct cc_oci_net_if_cfg *)
			g_slist_nth_data(config->net.interfaces, index);

		if (!cc_oci_tap_create(if_cfg->tap_device)) {
			return false;
		}

		#define NETLINK_CREATE_BRIDGE "link add name %s type bridge"
		g_snprintf(cmd_line, sizeof(cmd_line), NETLINK_CREATE_BRIDGE,
			if_cfg->bridge);
		if (!cc_oci_netlink_run(cmd_line)) {
			return false;
		}

		#define NETLINK_SET_MAC	"link set dev %s address 02:00:CA:FE:00:%.2x"
		/* TODO: Derive non conflicting mac from interface */
		g_snprintf(cmd_line, sizeof(cmd_line), NETLINK_SET_MAC,
			   if_cfg->ifname, index);
		if (!cc_oci_netlink_run(cmd_line)) {
			return false;
		}

		#define NETLINK_ADD_LINK_BR "link set dev %s master %s"
		g_snprintf(cmd_line, sizeof(cmd_line), NETLINK_ADD_LINK_BR,
			   if_cfg->ifname, if_cfg->bridge);
		if (!cc_oci_netlink_run(cmd_line)) {
			return false;
		}

		g_snprintf(cmd_line, sizeof(cmd_line), NETLINK_ADD_LINK_BR,
			if_cfg->tap_device, if_cfg->bridge);
		if (!cc_oci_netlink_run(cmd_line)) {
			return false;
		}

		#define NETLINK_LINK_EN	"link set dev %s up"
		g_snprintf(cmd_line, sizeof(cmd_line), NETLINK_LINK_EN,
			   if_cfg->tap_device);
		if (!cc_oci_netlink_run(cmd_line)) {
			return false;
		}

		g_snprintf(cmd_line, sizeof(cmd_line), NETLINK_LINK_EN,
			if_cfg->ifname);
		if (!cc_oci_netlink_run(cmd_line)) {
			return false;
		}

		g_snprintf(cmd_line, sizeof(cmd_line), NETLINK_LINK_EN,
			if_cfg->bridge);
		if (!cc_oci_netlink_run(cmd_line)) {
			return false;
		}
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
 * Count the number of valid bits in the subnet prefix
 * Copied over from BSD
 *
 * \param val \c the subnet
 * \param size \c size of the subnet value in bytes
 *
 * \return \c prefix length if valid else \c 0
 */
static guint
prefix(guchar *val, guint size)
{
        guchar *addr = val;
        guint byte, bit, plen = 0;

        for (byte = 0; byte < size; byte++, plen += 8) {
                if (addr[byte] != 0xff) {
                        break;
		}
	}

	if (byte == size) {
		return (plen);
	}

	for (bit = 7; bit != 0; bit--, plen++) {
                if (!(addr[byte] & (1 << bit))) {
                        break;
		}
	}

	/* Handle errors */
        for (; bit != 0; bit--) {
                if (addr[byte] & (1 << bit)) {
                        return(0);
		}
	}
        byte++;
        for (; byte < size; byte++) {
                if (addr[byte]) {
                        return(0);
		}
	}
        return (plen);
}

/*!
 * Obtain the Subnet prefix from subnet address
 *
 * \param family \c inetfamily
 * \param sin_addr \c inet address
 *
 * \return \c string containing subnet prefix
 */
static gchar *
subnet_to_prefix(const gint family, const void *const sin_addr) {
	guint pfix = 0;
	guchar *addr = (guchar *)sin_addr;

	if (!addr) {
		return g_strdup_printf("%d", pfix);
	}

	switch(family) {
	case AF_INET6:
		pfix = prefix(addr, 16);
	case AF_INET:
		pfix = prefix(addr, 4);
	}

	return g_strdup_printf("%d", pfix);
}

/*!
 * Obtain the string representation of the mac address
 * of an interface
 *
 * \param ifname \c interface name
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
 * \return \c string containing default gw on that interface, else NULL \c ""
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
		g_debug("unable to discover gateway on [%s] [%s]", ifname, output);
		goto out;
	}

	if (inet_pton(AF_INET, output_tokens[2], &(sa.sin_addr)) != 1) {
		g_critical("invalid gateway [%s] [%s]", output, output_tokens[2]);
		goto out;
	}

	gw = g_strdup(output_tokens[2]);
	g_debug("default gw [%s]", gw);

out:
	if (output_tokens != NULL) {
		g_strfreev(output_tokens);
	}

	if (output != NULL) {
		g_free(output);
	}

	return gw;
}

/*!
 * GCompareFunc for searching through the list 
 * of existing network interfaces
 *
 * \param[in] a \c a element from the list
 * \param[in] b \c a value to compare with
 *
 * \return negative value if a < b ; zero if a = b ; positive value if a > b
 */
gint static
compare_interface(gconstpointer a,
		  gconstpointer b) {
	const struct cc_oci_net_if_cfg *if_cfg = a;
	const gchar *ifname = b;

	return g_strcmp0(ifname, if_cfg->ifname);
}

/*!
 * Obtain the network configuration of the container
 * Currently done by by scanned the namespace
 * Ideally the OCI spec should be modified such that
 * these parameters are sent to the runtime
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
	struct cc_oci_net_if_cfg *if_cfg = NULL;
	struct cc_oci_net_cfg *net = NULL;
	gint family;
	gchar *ifname;

	if (!config) {
		return false;
	}

	net = &(config->net);

	if (getifaddrs(&ifaddrs) == -1) {
		g_critical("getifaddrs() failed with errno =  %d %s\n",
			errno, strerror(errno));
		return false;
	}

	g_debug("Discovering container interfaces");

	/* For now add the interfaces with a valid IPv4 address */
	for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
		GSList *elem;

		if (!ifa->ifa_addr) {
			continue;
		}

		g_debug("Interface := [%s]", ifa->ifa_name);

		family = ifa->ifa_addr->sa_family;
		if (!((family == AF_INET) || (family == AF_INET6)))  {
			continue;
		}

		if (!g_strcmp0(ifa->ifa_name, "lo")) {
			continue;
		}

		ifname = ifa->ifa_name;
		elem = g_slist_find_custom(net->interfaces,
					   ifname,
					   compare_interface);

		if (elem == NULL) {
			if_cfg = g_malloc0(sizeof(*if_cfg));
			if_cfg->ifname = g_strdup(ifname);
			if_cfg->mac_address = get_mac_address(ifname);
			if_cfg->tap_device = g_strdup_printf("c%s", ifname);
			if_cfg->bridge = g_strdup_printf("b%s", ifname);
			net->interfaces = g_slist_append(net->interfaces, if_cfg);
		} else {
			if_cfg = (struct cc_oci_net_if_cfg  *) elem->data;
		}

		if (family == AF_INET) {
			struct cc_oci_net_ipv4_cfg *ipv4_cfg;
			ipv4_cfg = g_malloc0(sizeof(*ipv4_cfg));

			if_cfg->ipv4_addrs = g_slist_append(
				if_cfg->ipv4_addrs, ipv4_cfg);

			ipv4_cfg->ip_address = get_address(family,
				&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr);
			ipv4_cfg->subnet_mask = get_address(family,
				&((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr);

			if (net->gateway == NULL) {
				net->gateway = get_default_gw(ifname);
			}

		} else if (family == AF_INET6) {
			struct cc_oci_net_ipv6_cfg *ipv6_cfg;
			ipv6_cfg = g_malloc0(sizeof(*ipv6_cfg));
			if_cfg->ipv6_addrs = g_slist_append(
				if_cfg->ipv6_addrs, ipv6_cfg);

			ipv6_cfg->ipv6_address = get_address(family,
				&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr);
			ipv6_cfg->ipv6_prefix = subnet_to_prefix(family,
				&((struct sockaddr_in6 *)ifa->ifa_netmask)->sin6_addr);
		}
	}

	freeifaddrs(ifaddrs);

	if (config->oci.hostname){
		net->hostname = g_strdup(config->oci.hostname);
	} else {
		net->hostname = g_strdup("");
	}

	if (net->gateway == NULL) {
		net->gateway = g_strdup("");
	}

	/* TODO: Need to see if this needed, does resolv.conf handle this */
	net->dns_ip1 = g_strdup("");
	net->dns_ip2 = g_strdup("");

	g_debug("[%d] networks discovered", g_slist_length(net->interfaces));

	return true;
}
