/* libsocketcan-utils.h
 *
 * (C) 2009 Luotao Fu <l.fu@pengutronix.de>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#ifndef LIBSOCKETCAN_UTILS_H
#define LIBSOCKETCAN_UTILS_H

#ifdef HAVE_CONFIG_H
#include "libsocketcan_config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>

#include <linux/if_link.h>
#include <linux/rtnetlink.h>
#include <linux/netlink.h>

#include <libsocketcan.h>

/* Define DISABLE_ERROR_LOG to disable printing of error messages to stderr. */
#ifdef DISABLE_ERROR_LOG
#define perror(x)				while (0) { perror(x); }
#define fprintf(stream, format, args...)	while (0) { fprintf(stream, format, ##args); }
#endif

#define NLMSG_TAIL(nmsg) \
	((struct rtattr *) (((void *) (nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

int addattr32(struct nlmsghdr *n, size_t maxlen, int type, __u32 data);
int addattr_l(struct nlmsghdr *n, size_t maxlen, int type, const void *data, int alen);
#endif //#ifndef LIBSOCKETCAN_UTILS_H