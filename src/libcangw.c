/* libcangw.c
 *
 * (C) 2025 Naoto Yamaguchi <naoto.yamaguchi@aisin.co.jp>
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

/**
 * @file
 * @brief Socket CAN gateway library
 */

#include <linux/can/gw.h>
#include <sys/socket.h>

#include "libsocketcan-utils.h"

#include <libsocketcangw.h>

struct s_request_data {
	struct nlmsghdr nh;
	struct rtcanmsg rtcan;
	char buf[1500];
};

static int send_cangw_request(struct s_request_data *req)
{
	int result = 0;
	int ret = -1;
	int sock_fd = -1;
	struct nlmsghdr *nlh = NULL;
	struct nlmsgerr *rte = NULL;
	struct sockaddr_nl nladdr;
	unsigned char rxbuf[8192];

	// Open netlink socket interface
	sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (sock_fd < 0) {
		result = -1;
		goto do_return;
	}

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid    = 0;
	nladdr.nl_groups = 0;

	ret = sendto(sock_fd, req, req->nh.nlmsg_len, 0, (struct sockaddr*)&nladdr, sizeof(nladdr));
	if (ret < 0) {
		result = -1;
		goto do_return;
	}

	memset(rxbuf, 0, sizeof(rxbuf));
	ret = recv(sock_fd, &rxbuf, sizeof(rxbuf), 0);
	if (ret < 0) {
		result = -1;
		goto do_return;
	}

	nlh = (struct nlmsghdr *)rxbuf;
	if (nlh->nlmsg_type != NLMSG_ERROR) {
		result = -2;
		goto do_return;
	}

	rte = (struct nlmsgerr *)NLMSG_DATA(nlh);
	if (rte->error < 0) {
		result = -3;
	}

do_return:
	if (sock_fd >= 0) {
		close(sock_fd);
	}

	return result;
}
/**
 * @ingroup intern
 * @brief init_req_data - initialize for req data
 *
 * @param req pointer to s_request_data structure that is initialized by this function.
 * @param flags value of the nlmsg_flags
 * @param type value of the nlmsg_type
 *
 * Set a netlink request data from rule to req.
 */
static void init_req_data(struct s_request_data *req, unsigned short flags, unsigned short type)
{
	// Setup common message
	memset(req, 0, sizeof(struct s_request_data));

	req->nh.nlmsg_flags = flags;
	req->nh.nlmsg_type  = type;
	req->nh.nlmsg_len   = NLMSG_LENGTH(sizeof(struct rtcanmsg));
	req->nh.nlmsg_seq   = 0;

	req->rtcan.can_family  = AF_CAN;
	req->rtcan.gwtype = CGW_TYPE_CAN_CAN;
	req->rtcan.flags = 0;
}

/**
 * @ingroup intern
 * @brief operate_rule_options - operate to options of gw rule
 *
 * @param req pointer to s_request_data structure that is wrote the options.
 * @param rule pointer to source data of the gw configuration rule
 *
 * Set a netlink request data from rule to req.
 */
static void operate_rule_options(struct s_request_data *req, socketcan_gw_rule_t *rule)
{
	if ((rule->options & SOCKETCAN_GW_RULE_ECHO) == SOCKETCAN_GW_RULE_ECHO) {
		if (rule->echo == 1) {
			req->rtcan.flags |= CGW_FLAGS_CAN_ECHO;
		}
	}

	if ((rule->options & SOCKETCAN_GW_RULE_FILTER) == SOCKETCAN_GW_RULE_FILTER) {
		addattr_l(&req->nh, sizeof(struct s_request_data), CGW_FILTER, &rule->filter, sizeof(struct can_filter));
	}
}

/**
 * @ingroup extern
 * cangw_add_rule - add routing rule to can gateway
 * @param rule rule data of the can gateway.
 *
 * @return 0 if success
 * @return -1 if operation is failed
 * @return -2 if linux does not support can gateway
 * @return -3 if argument is invalid
 */
int cangw_add_rule(socketcan_gw_rule_t *rule)
{
	int result = 0;
	int ret = -1;
	struct s_request_data req;

	if (rule == NULL) {
		result = -3;
		goto do_return;
	}

	// Setup common message
	init_req_data(&req, (NLM_F_REQUEST | NLM_F_ACK), RTM_NEWROUTE);

	if ((rule->src_ifindex == 0) || (rule->dst_ifindex == 0)) {
		// invalid ifindex
		result = -3;
		goto do_return;
	}
	addattr_l(&req.nh, sizeof(req), CGW_SRC_IF, &rule->src_ifindex, sizeof(rule->src_ifindex));
	addattr_l(&req.nh, sizeof(req), CGW_DST_IF, &rule->dst_ifindex, sizeof(rule->dst_ifindex));

	// operate options
	operate_rule_options(&req, rule);

	ret = send_cangw_request(&req);
	if (ret < 0) {
		result = -1;
		goto do_return;
	}

do_return:
	return result;
}

/**
 * @ingroup extern
 * cangw_delete_rule - delete routing rule to can gateway
 * @param rule rule data of the can gateway.
 *
 * @return 0 if success
 * @return -1 if operation is failed
 * @return -2 if linux does not support can gateway
 * @return -3 if argument is invalid
 */
int cangw_delete_rule(socketcan_gw_rule_t *rule)
{
	int result = 0;
	int ret = -1;
	struct s_request_data req;

	if (rule == NULL) {
		result = -3;
		goto do_return;
	}

	// Setup common message
	init_req_data(&req, (NLM_F_REQUEST | NLM_F_ACK), RTM_DELROUTE);

	if ((rule->src_ifindex == 0) || (rule->dst_ifindex == 0)) {
		// invalid ifindex
		result = -3;
		goto do_return;
	}
	addattr_l(&req.nh, sizeof(req), CGW_SRC_IF, &rule->src_ifindex, sizeof(rule->src_ifindex));
	addattr_l(&req.nh, sizeof(req), CGW_DST_IF, &rule->dst_ifindex, sizeof(rule->dst_ifindex));

	// operate options
	operate_rule_options(&req, rule);

	ret = send_cangw_request(&req);
	if (ret < 0) {
		result = -1;
		goto do_return;
	}

do_return:
	return result;
}

/**
 * @ingroup extern
 * cangw_clean_rule - delete all routing rule to can gateway
 *
 * @return 0 if success
 * @return -1 if operation is failed
 * @return -2 if linux does not support can gateway
 */
int cangw_clean_rule(void)
{
	int result = 0;
	int ret = -1;
	unsigned int ifindex = 0;
	struct s_request_data req;

	// Setup common message
	init_req_data(&req, (NLM_F_REQUEST | NLM_F_ACK), RTM_DELROUTE);

	// If src and dst ifindex set to 0, the all rule are deleted.
	addattr_l(&req.nh, sizeof(req), CGW_SRC_IF, &ifindex, sizeof(ifindex));
	addattr_l(&req.nh, sizeof(req), CGW_DST_IF, &ifindex, sizeof(ifindex));

	ret = send_cangw_request(&req);
	if (ret < 0) {
		result = -1;
		goto do_return;
	}

do_return:
	return result;
}