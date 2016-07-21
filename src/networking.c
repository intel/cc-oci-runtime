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

/*!
 * Request to create a named tap interface
 *
 * \param tap \c tap interface name to create
 *
 * \return \c true on success, else \c false.
 */
static gboolean
cc_oci_tap_create (const gchar *tap)
{
	struct ifreq ifr;
	int fd;

	if( (fd = open(TUNDEV, O_RDWR)) < 0 ) {
		g_critical("Failed to open [%s] [%s]", TUNDEV, strerror(errno));
		return false;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP;
	g_strlcpy(ifr.ifr_name, tap, IFNAMSIZ);

	if( ioctl(fd, TUNSETIFF, (void *) &ifr) < 0 ) {
		g_critical("Failed to create tap [%s] [%s]",
			   tap, strerror(errno));
		close(fd);
		return false;
	}

	if( ioctl(fd, TUNSETPERSIST, 1) < 0 ){
		g_critical("failed to TUNSETPERSIST [%s] [%s]",
			   tap, strerror(errno));
		close(fd);
		return false;
	}

	close(fd);
	return true;
}


static gboolean
cc_oci_netlink_run(const gchar *cmd_line) {
	gint exit_status = -1;
	gboolean ret = false;

	ret = g_spawn_command_line_sync(cmd_line,
				  NULL,
				  NULL,
				  &exit_status,
				  NULL);

	if ( !ret ) {
		g_critical("failed to spawn [%s]", cmd_line);
		return false;
	}

	if (exit_status) {
		g_critical("netlink failure [%s] [%d]", cmd_line, exit_status);
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
 * \param netconfig \c networking configuration
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_network_create (const struct cc_oci_config * const config) {
	gchar cmd_line[1024];

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

#define NETLINK_CREATE_BRIDGE "ip link add name %s type bridge"
	g_snprintf(cmd_line, sizeof(cmd_line),
		   NETLINK_CREATE_BRIDGE,
		   config->net.bridge);

	if ( !cc_oci_netlink_run(cmd_line) ) {
		return false;
	}

#define NETLINK_SET_MAC	"ip link set dev %s address %s"
	//TODO: Derive non conflicting mac from interface */
	g_snprintf(cmd_line, sizeof(cmd_line),
		   NETLINK_SET_MAC,
		   config->net.ifname,
		   "02:00:CA:FE:00:01");

	if ( !cc_oci_netlink_run(cmd_line) ) {
		return false;
	}

#define NETLINK_ADD_LINK_BR "ip link set dev %s master %s"
	g_snprintf(cmd_line, sizeof(cmd_line),
		   NETLINK_ADD_LINK_BR,
		   config->net.ifname,
		   config->net.bridge);

	if ( !cc_oci_netlink_run(cmd_line) ) {
		return false;
	}

	g_snprintf(cmd_line, sizeof(cmd_line),
		   NETLINK_ADD_LINK_BR,
		   config->net.tap_device,
		   config->net.bridge);

	if ( !cc_oci_netlink_run(cmd_line) ) {
		return false;
	}

#define NETLINK_LINK_EN	"ip link set dev %s up"
	g_snprintf(cmd_line, sizeof(cmd_line),
		   NETLINK_LINK_EN,
		   config->net.tap_device);

	if ( !cc_oci_netlink_run(cmd_line) ) {
		return false;
	}

	g_snprintf(cmd_line, sizeof(cmd_line),
		   NETLINK_LINK_EN,
		   config->net.ifname);

	if ( !cc_oci_netlink_run(cmd_line) ) {
		return false;
	}

	g_snprintf(cmd_line, sizeof(cmd_line),
		   NETLINK_LINK_EN,
		   config->net.bridge);

	if ( !cc_oci_netlink_run(cmd_line) ) {
		return false;
	}

	return true;
}

static gchar *
get_address(const gint family, const void * const sin_addr)
{
	gchar addrBuf[INET6_ADDRSTRLEN];

	inet_ntop(family, sin_addr, addrBuf, sizeof(addrBuf));

	return(g_strdup(addrBuf));
}

static gchar *
get_mac_address (const gchar * const ifname)
{
	struct ifreq ifr;
	int fd = -1;
	gchar *macaddr;
	gchar *data;

	fd = socket (AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (fd < 0) {
		g_critical("socket() failed with errno =  %d %s \n",
			errno, strerror(errno));
		return(g_strdup(""));
	}

	memset (&ifr, 0, sizeof (struct ifreq));
	g_strlcpy (ifr.ifr_name, ifname, IFNAMSIZ);

	if (ioctl (fd, SIOCGIFHWADDR, &ifr) < 0) {
		g_critical("ioctl() failed with errno =  %d %s \n",
			errno, strerror(errno));
		macaddr = g_strdup("");
		goto out;
	}

	if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
		g_critical("invalid interface  %s \n", ifname);
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
 * Obtain the network configuration of a particular
 * interface by querying the network namespace.
 * Will be scanned from the namespace
 * Ideally the OCI spec should be modified such that
 * these parameters are sent to the runtime 
 *
 * \param[in] ifname \c interface name
 * \param[in,out] config \ref cc_oci_config.
 *
 * \return \c true on success, else \c false.
 */
gboolean
cc_oci_network_discover (const gchar * const ifname, 
			  struct cc_oci_config * const config)
{
	struct ifaddrs *ifa = NULL;
	struct ifaddrs *ifaddrs = NULL;
	gint family;

	if ((! config) || (! ifname)) {
		return false;
	}


	if (getifaddrs(&ifaddrs) == -1) {
		g_critical("getifaddrs() failed with errno =  %d %s \n",
			errno, strerror(errno));
		return false;
	}

	g_debug("Discovering container interfaces");
	for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL) {
			continue;
		}

		g_debug("	Interface := [%s]", ifa->ifa_name);
		if (g_strcmp0(ifa->ifa_name, ifname) != 0) {
			continue;
		}


		family = ifa->ifa_addr->sa_family;
		if ((family != AF_INET) && (family != AF_INET6)) {
			continue;
		}

		/* We have found at least one interface that matches */
		config->net.ifname = g_strdup (ifname);

		/* Assuming interfaces has both IPv4 and IPv6 addrs */
		if ( family == AF_INET6 ) {
			config->net.ipv6_address = get_address(family,
			&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr);

			g_debug("Container IPv6 IP := [%s]", 
				config->net.ipv6_address);

			continue;
		}

		config->net.ip_address = get_address(family,
			&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr);
		g_debug("Container IP := [%s]", config->net.ip_address);

		if(ifa->ifa_netmask != NULL) {
			config->net.subnet_mask = get_address(family,
			&((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr);
		} else {
			config->net.subnet_mask = g_strdup ("");
		}

		g_debug("Container Subnet := [%s]", config->net.subnet_mask);

		config->net.mac_address = get_mac_address(ifname);
		g_debug("Container mac := [%s]", config->net.mac_address);

		config->net.gateway = g_strdup ("");

		/* FIXME: Use a generated names as the prepend may exceed max length */
		config->net.tap_device = g_strdup_printf ("c%s", ifname);
		config->net.bridge = g_strdup_printf ("b%s", ifname);
	}

	freeifaddrs(ifaddrs);

	if (config->oci.hostname != NULL) {
		config->net.hostname = g_strdup (config->oci.hostname);
	} else {
		config->net.hostname = g_strdup ("");
	}
	g_debug("Container Hostname := [%s]", config->net.hostname);


	/* FIXME: fake it for now */
	/* TODO: We need to support multiple networks */
#if 1
	config->net.dns_ip1 = g_strdup ("");
	config->net.dns_ip2 = g_strdup ("");
	g_critical ("FIXME: faking network config setup");
#endif

	return true;
}
