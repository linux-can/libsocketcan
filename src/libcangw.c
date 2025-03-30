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

#define RTCAN_RTA(r)  ((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct rtcanmsg))))
#define RTCAN_PAYLOAD(n) NLMSG_PAYLOAD(n,sizeof(struct rtcanmsg))

/**
 * @ingroup intern
 * send_cangw_set_request - send request to add gw rule into kernel
 * @param req pointer to request data.
 *
 * @return 0 if success
 * @return -1 if operation is failed
 * @return -2 if linux does not support can gateway
 * @return -3 if returned fail response
 */
static int send_cangw_set_request(struct s_request_data *req)
{
	int result = 0;
	int sock_fd = -1;
	ssize_t ret = -1;
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
 * push_gw_rule - push gw rule into gw_rules
 * @param gw_rules pointer to rules structure of the can gateway.
 * @param rule pointer to new rule to push into gw_rules.
 *
 * @return 0 if success
 * @return -1 if operation is failed
 */
static int push_gw_rule(socketcan_gw_rules_t *gw_rules, socketcan_gw_rule_t *rule)
{
	int result = 0;

	if (gw_rules->rules == NULL) {
		// Create rules array
		gw_rules->rule_num = 0;	// Initial size
		gw_rules->array_num = 2;	// Initial size
		gw_rules->rules = (socketcan_gw_rule_t**)malloc(sizeof(socketcan_gw_rule_t*) * gw_rules->array_num);
		if (gw_rules->rules == NULL) {
			result = -1;
			goto do_return;
		}
	}

	if (!(gw_rules->rule_num < gw_rules->array_num)) {
		// Extend array
		socketcan_gw_rule_t **pnew_rules = NULL;
		gw_rules->array_num = gw_rules->array_num * 2;
		pnew_rules = (socketcan_gw_rule_t**)realloc(gw_rules->rules, (sizeof(socketcan_gw_rule_t*) * gw_rules->array_num));
		if (pnew_rules != NULL) {
			gw_rules->rules = pnew_rules;
		} else {
			result = -1;
			goto do_return;
		}
	}

	gw_rules->rules[gw_rules->rule_num] = rule;
	gw_rules->rule_num = gw_rules->rule_num + 1;

do_return:
	return result;
}

/**
 * @ingroup intern
 * free_gw_rules - free memory of gw_rules
 * @param gw_rules pointer to rules structure of the can gateway.
 *
 * @return 0 if success
 */
static int free_gw_rules(socketcan_gw_rules_t *gw_rules)
{
	for(size_t i=0; i < gw_rules->rule_num; i++) {
		free(gw_rules->rules[i]);
		gw_rules->rules[i] = NULL;
	}

	free(gw_rules->rules);
	gw_rules->rules = NULL;
	gw_rules->rule_num = 0;
	gw_rules->array_num = 0;

	return 0;
}

/**
 * @ingroup intern
 * parse_listing_data - parse routing data that get from kernel
 * @param gw_rules rules pointer to rules structure of the can gateway to add rule element.
 * @param rxbuf buffer of received data from kernel.
 * @param len buffer length of received data from kernel.
 *
 * @return 1 if completed to get routing rule from kernel
 * @return 0 if end of received data
 * @return -1 if operation is failed
 */
static int parse_listing_data(socketcan_gw_rules_t *gw_rules, unsigned char *rxbuf, int len)
{
	socketcan_gw_rule_t *rule = NULL;
	struct rtcanmsg *rtc = NULL;
	struct rtattr *rta = NULL;
	struct nlmsghdr *nlh = NULL;
	int rtlen = 0;
	int result = 0;

	nlh = (struct nlmsghdr*)rxbuf;

	while (1) {
		if (!NLMSG_OK(nlh, len)){
			result = 0;
			break;
		}

		if (nlh->nlmsg_type == NLMSG_ERROR) {
			result = -1;
			break;
		}

		if (nlh->nlmsg_type == NLMSG_DONE) {
			result = 1;
			break;
		}

		rtc = (struct rtcanmsg *)NLMSG_DATA(nlh);
		if (rtc->can_family != AF_CAN) {
			result = -1;
			break;
		}

		if (rtc->gwtype != CGW_TYPE_CAN_CAN) {
			result = -1;
			break;
		}

		rule = (socketcan_gw_rule_t*)malloc(sizeof(socketcan_gw_rule_t));
		if (rule == NULL) {
			result = -1;
			goto error_return;
		}
		memset(rule, 0 ,sizeof(socketcan_gw_rule_t));

		rta = (struct rtattr *) RTCAN_RTA(rtc);
		rtlen = RTCAN_PAYLOAD(nlh);
		while (RTA_OK(rta, rtlen)) {
			switch(rta->rta_type) {
			case CGW_SRC_IF:
				rule->src_ifindex = (*(unsigned int*)RTA_DATA(rta));
				break;
			case CGW_DST_IF:
				rule->dst_ifindex = (*(unsigned int*)RTA_DATA(rta));
				break;
			default:
				break;
			}
			rta = RTA_NEXT(rta, rtlen);
		}

		rule->options = (SOCKETCAN_GW_RULE_ECHO | SOCKETCAN_GW_RULE_FILTER);

		if ((rtc->flags & CGW_FLAGS_CAN_ECHO) == CGW_FLAGS_CAN_ECHO) {
			rule->echo = 1;
		} else {
			rule->echo = 0;
		}

		rta = (struct rtattr *) RTCAN_RTA(rtc);
		rtlen = RTCAN_PAYLOAD(nlh);
		while(RTA_OK(rta, rtlen)) {
			switch(rta->rta_type) {
			case CGW_FILTER:
			{
				struct can_filter *filter = (struct can_filter *)RTA_DATA(rta);
				if (filter->can_id & CAN_INV_FILTER) {
					rule->filter.can_id = (filter->can_id & ~CAN_INV_FILTER);
				} else {
					rule->filter.can_id = filter->can_id;
				}
				rule->filter.can_mask = filter->can_mask;
				break;
			}

			default:
				break;
			}
			rta = RTA_NEXT(rta, rtlen);
		}

		push_gw_rule(gw_rules, rule);

		nlh = NLMSG_NEXT(nlh, len);
	}

	return result;

error_return:
	(void) free(rule);
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

	ret = send_cangw_set_request(&req);
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

	ret = send_cangw_set_request(&req);
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

	ret = send_cangw_set_request(&req);
	if (ret < 0) {
		result = -1;
		goto do_return;
	}

do_return:
	return result;
}

/**
 * @ingroup extern
 * cangw_get_rules - get can gateway routing rule
 * @param gw_rules double pointer to rules structure of the can gateway to get existing rules.
 *
 * @return 0 if success
 * @return -1 if operation is failed
 * @return -2 if linux does not support can gateway
 * @return -3 if argument is invalid
 */
int cangw_get_rules(socketcan_gw_rules_t **gw_rules)
{
	int result = 0;
	int sock_fd = -1;
	ssize_t ret = -1;
	struct s_request_data req;
	struct sockaddr_nl nladdr;
	socketcan_gw_rules_t *pgw_rules = NULL;

	if (gw_rules == NULL) {
		result = -3;
		goto do_return;
	}

	// Setup common message
	init_req_data(&req, (NLM_F_REQUEST | NLM_F_DUMP), RTM_GETROUTE);

	// Open netlink socket interface
	sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (sock_fd < 0) {
		result = -2;
		goto do_return;
	}

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid    = 0;
	nladdr.nl_groups = 0;

	ret = sendto(sock_fd, &req, req.nh.nlmsg_len, 0, (struct sockaddr*)&nladdr, sizeof(nladdr));
	if (ret < 0) {
		result = -1;
		goto do_return;
	}

	pgw_rules = malloc(sizeof(socketcan_gw_rules_t));
	if (pgw_rules == NULL) {
		result = -1;
		goto do_return;
	}

	pgw_rules->rule_num = 0;
	pgw_rules->array_num = 0;
	pgw_rules->rules = NULL;

	while (1) {
		unsigned char rxbuf[8192];
		memset(rxbuf, 0, sizeof(rxbuf));

		ret = recv(sock_fd, &rxbuf, sizeof(rxbuf), 0);
		if (ret < 0) {
			result = -1;
			goto do_return;
		}

		/* leave on errors or NLMSG_DONE */
		if (parse_listing_data(pgw_rules, rxbuf, ret))
			break;
	}
	print_gw_rules(pgw_rules);
	(*gw_rules) = pgw_rules;

do_return:
	if (sock_fd >= 0) {
		close(sock_fd);
	}

	return result;
}
/**
 * @ingroup extern
 * cangw_release_rules - delete routing rule to can gateway
 * @param gw_rules rules pointer to rules structure of the can gateway to free allocated memory.
 *
 * @return 0 if success
 * @return -3 if argument is invalid
 */
int cangw_release_rules(socketcan_gw_rules_t *gw_rules)
{
	int result = 0;

	if (gw_rules == NULL) {
		result = -3;
		goto do_return;
	}

	(void) free_gw_rules(gw_rules);
	(void) free(gw_rules);

do_return:
	return result;
}