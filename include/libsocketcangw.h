/*
 * libsocketcangw.h
 *
 * (C) 2025 Naoto Yamaguchi <naoto.yamaguchi@aisin.co.jp>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or any later version.
 *
 * This library is distributed in the hope that it will be useful, but without
 * any warranty; without even the implied warranty of merchantability or fitness
 * for a particular purpose. see the gnu lesser general public license for more
 * details.
 *
 * you should have received a copy of the gnu lesser general public license
 * along with this library; if not, write to the free software foundation, inc.,
 * 59 temple place, suite 330, boston, ma 02111-1307 usa
 */

 #ifndef _socketcangw_netlink_h
 #define _socketcangw_netlink_h
 
 /**
  * @file
  * @brief API overview
  */
 #include <linux/can.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif

#define SOCKETCAN_GW_RULE_ECHO		(0x00000001U)
#define SOCKETCAN_GW_RULE_FILTER	(0x00000002U)

struct s_socketcan_gw_rule {
	unsigned int src_ifindex;
	unsigned int dst_ifindex;

	unsigned int	options;

	unsigned int	echo;
	struct can_filter filter;
 };
typedef struct s_socketcan_gw_rule socketcan_gw_rule_t;
 

int cangw_add_rule(socketcan_gw_rule_t *rule);
int cangw_delete_rule(socketcan_gw_rule_t *rule);
int cangw_clean_rule(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif