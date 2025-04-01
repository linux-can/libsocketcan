#include "libsocketcan-utils.c"
#include "libcangw.c"
#include <setjmp.h>
#include <cmocka.h>
#include <getopt.h>

int print_gw_rules(socketcan_gw_rules_t *gw_rules)
{
	char src_ifname[IF_NAMESIZE];
	char dst_ifname[IF_NAMESIZE];

	for(size_t i=0; i < gw_rules->rule_num; i++) {
		socketcan_gw_rule_t *rule = gw_rules->rules[i];

		fprintf(stdout, "cangw: -s %s -d %s -f %03X:%X echo=%d\n",
			if_indextoname(rule->src_ifindex, src_ifname),
			if_indextoname(rule->dst_ifindex, dst_ifname),
			rule->filter.can_id,
			rule->filter.can_mask,
			rule->echo
		);
	}

	return 0;
}
//------------------------------------------------------------------------------------------
static int rule_setup(void **state) {
	int ret = -1;
	unsigned long ifindex_tmp = 0;

	ifindex_tmp = if_nametoindex("vcan0");
	if (ifindex_tmp == 0) {
		fprintf(stderr,"Did not create vcan0 interface. That interface needs in test.\n");
		return -1;
	}

	ifindex_tmp = if_nametoindex("vcan1");
	if (ifindex_tmp == 0) {
		fprintf(stderr,"Did not create vcan1 interface. That interface needs in test.\n");
		return -1;
	}

	ifindex_tmp = if_nametoindex("vxcan0");
	if (ifindex_tmp == 0) {
		fprintf(stderr,"Did not create vxcan0 interface. That interface needs in test.\n");
		return -1;
	}

	ret = cangw_clean_rule();
	if (ret < 0) {
		fprintf(stderr,"Could not clean rules.\n");
		return -1;
	}

	return 0;
}

static int rule_teardown(void **state) {
	int ret = -1;

	ret = cangw_clean_rule();
	if (ret < 0) {
		fprintf(stderr,"Could not clean rules.\n");
		return -1;
	}

	return 0;
}
//------------------------------------------------------------------------------------------
static void add_rule_test_10(void **state) {
	int ret = -1;
	socketcan_gw_rule_t gw_rule;
	socketcan_gw_rules_t *gw_rules = NULL;

	memset(&gw_rule, 0, sizeof(gw_rule));

	gw_rule.src_ifindex = if_nametoindex("vcan0");
	gw_rule.dst_ifindex = if_nametoindex("vcan1");

	gw_rule.options |= SOCKETCAN_GW_RULE_ECHO;
	gw_rule.echo = 1;

	gw_rule.options |= SOCKETCAN_GW_RULE_FILTER;
	gw_rule.filter.can_id = 0x3C0;
	gw_rule.filter.can_mask = 0xff0;

	ret = cangw_add_rule(&gw_rule);
	assert_true(ret == 0);

	ret = cangw_get_rules(&gw_rules);
	assert_true(ret == 0);
	assert_non_null(gw_rules);
	assert_true(gw_rules->rule_num == 1);
	assert_true(gw_rules->rules[0]->src_ifindex == if_nametoindex("vcan0"));
	assert_true(gw_rules->rules[0]->dst_ifindex == if_nametoindex("vcan1"));
	assert_true(gw_rules->rules[0]->options == (SOCKETCAN_GW_RULE_ECHO | SOCKETCAN_GW_RULE_FILTER));
	assert_true(gw_rules->rules[0]->echo == 1);
	assert_true(gw_rules->rules[0]->filter.can_id == 0x3C0);
	assert_true(gw_rules->rules[0]->filter.can_mask == 0xff0);

	cangw_release_rules(gw_rules);
}
//------------------------------------------------------------------------------------------
static void add_rule_test_11(void **state) {
	int ret = -1;
	socketcan_gw_rule_t gw_rule;
	socketcan_gw_rules_t *gw_rules = NULL;

	memset(&gw_rule, 0, sizeof(gw_rule));

	gw_rule.src_ifindex = if_nametoindex("vcan0");
	gw_rule.dst_ifindex = if_nametoindex("vxcan0");

	gw_rule.options |= SOCKETCAN_GW_RULE_ECHO;
	gw_rule.echo = 0;

	gw_rule.options |= SOCKETCAN_GW_RULE_FILTER;
	gw_rule.filter.can_id = 0x3C0;
	gw_rule.filter.can_mask = 0xff0;

	ret = cangw_add_rule(&gw_rule);
	assert_true(ret == 0);

	ret = cangw_get_rules(&gw_rules);
	assert_true(ret == 0);
	assert_non_null(gw_rules);
	assert_true(gw_rules->rule_num == 1);
	assert_true(gw_rules->rules[0]->src_ifindex == if_nametoindex("vcan0"));
	assert_true(gw_rules->rules[0]->dst_ifindex == if_nametoindex("vxcan0"));
	assert_true(gw_rules->rules[0]->options == (SOCKETCAN_GW_RULE_ECHO | SOCKETCAN_GW_RULE_FILTER));
	assert_true(gw_rules->rules[0]->echo == 0);
	assert_true(gw_rules->rules[0]->filter.can_id == 0x3C0);
	assert_true(gw_rules->rules[0]->filter.can_mask == 0xff0);

	cangw_release_rules(gw_rules);
}
//------------------------------------------------------------------------------------------
static void add_rule_test_12(void **state) {
	int ret = -1;
	socketcan_gw_rule_t gw_rule;
	socketcan_gw_rules_t *gw_rules = NULL;

	memset(&gw_rule, 0, sizeof(gw_rule));

	gw_rule.src_ifindex = if_nametoindex("vxcan0");
	gw_rule.dst_ifindex = if_nametoindex("vcan0");

	gw_rule.options |= SOCKETCAN_GW_RULE_ECHO;
	gw_rule.echo = 0;

	gw_rule.options |= SOCKETCAN_GW_RULE_FILTER;
	gw_rule.filter.can_id = 0x188;
	gw_rule.filter.can_mask = 0xfff;

	ret = cangw_add_rule(&gw_rule);
	assert_true(ret == 0);

	ret = cangw_get_rules(&gw_rules);
	assert_true(ret == 0);
	assert_non_null(gw_rules);
	assert_true(gw_rules->rule_num == 1);
	assert_true(gw_rules->rules[0]->src_ifindex == if_nametoindex("vxcan0"));
	assert_true(gw_rules->rules[0]->dst_ifindex == if_nametoindex("vcan0"));
	assert_true(gw_rules->rules[0]->options == (SOCKETCAN_GW_RULE_ECHO | SOCKETCAN_GW_RULE_FILTER));
	assert_true(gw_rules->rules[0]->echo == 0);
	assert_true(gw_rules->rules[0]->filter.can_id == 0x188);
	assert_true(gw_rules->rules[0]->filter.can_mask == 0xfff);

	cangw_release_rules(gw_rules);
}
//------------------------------------------------------------------------------------------
static void add_rule_test_13(void **state) {
	int ret = -1;
	socketcan_gw_rule_t gw_rule[2];
	socketcan_gw_rules_t *gw_rules = NULL;

	memset(&gw_rule[0], 0, sizeof(gw_rule[0]));
	memset(&gw_rule[1], 0, sizeof(gw_rule[1]));

	gw_rule[0].src_ifindex = if_nametoindex("vcan0");
	gw_rule[0].dst_ifindex = if_nametoindex("vcan1");
	gw_rule[0].options |= SOCKETCAN_GW_RULE_ECHO;
	gw_rule[0].echo = 1;
	gw_rule[0].options |= SOCKETCAN_GW_RULE_FILTER;
	gw_rule[0].filter.can_id = 0x001;
	gw_rule[0].filter.can_mask = 0x0ff;

	gw_rule[1].src_ifindex = if_nametoindex("vxcan0");
	gw_rule[1].dst_ifindex = if_nametoindex("vcan0");
	gw_rule[1].options |= SOCKETCAN_GW_RULE_FILTER;
	gw_rule[1].filter.can_id = 0x00f;
	gw_rule[1].filter.can_mask = 0xff0;

	ret = cangw_add_rule(&gw_rule[1]);
	assert_true(ret == 0);

	ret = cangw_add_rule(&gw_rule[0]);
	assert_true(ret == 0);

	ret = cangw_get_rules(&gw_rules);
	assert_true(ret == 0);
	assert_non_null(gw_rules);
	assert_true(gw_rules->rule_num == 2);
	assert_true(gw_rules->rules[0]->src_ifindex == if_nametoindex("vcan0"));
	assert_true(gw_rules->rules[0]->dst_ifindex == if_nametoindex("vcan1"));
	assert_true(gw_rules->rules[0]->options == (SOCKETCAN_GW_RULE_ECHO | SOCKETCAN_GW_RULE_FILTER));
	assert_true(gw_rules->rules[0]->echo == 1);
	assert_true(gw_rules->rules[0]->filter.can_id == 0x001);
	assert_true(gw_rules->rules[0]->filter.can_mask == 0x0ff);
	assert_true(gw_rules->rules[1]->src_ifindex == if_nametoindex("vxcan0"));
	assert_true(gw_rules->rules[1]->dst_ifindex == if_nametoindex("vcan0"));
	assert_true(gw_rules->rules[1]->options == (SOCKETCAN_GW_RULE_ECHO | SOCKETCAN_GW_RULE_FILTER));
	assert_true(gw_rules->rules[1]->echo == 0);
	assert_true(gw_rules->rules[1]->filter.can_id == 0x00f);
	assert_true(gw_rules->rules[1]->filter.can_mask == 0xff0);

	cangw_release_rules(gw_rules);
}
//------------------------------------------------------------------------------------------
static void add_rule_test_14(void **state) {
	int ret = -1;
	socketcan_gw_rule_t gw_rule;
	socketcan_gw_rules_t *gw_rules = NULL;

	memset(&gw_rule, 0, sizeof(gw_rule));

	gw_rule.src_ifindex = if_nametoindex("vcan0");
	gw_rule.dst_ifindex = if_nametoindex("vcan1");

	gw_rule.options |= SOCKETCAN_GW_RULE_ECHO;
	gw_rule.echo = 1;

	gw_rule.options |= SOCKETCAN_GW_RULE_FILTER;

	canid_t id = 1;
	for(int i=0;i < 256;i++) {
		gw_rule.filter.can_id = id;
		gw_rule.filter.can_mask = 0xfff;
		ret = cangw_add_rule(&gw_rule);
		assert_true(ret == 0);		
		id++;
	}

	ret = cangw_get_rules(&gw_rules);
	assert_true(ret == 0);
	assert_non_null(gw_rules);
	assert_true(gw_rules->rule_num == 256);
	for(int i=0;i < 256;i++) {
		id--;
		assert_true(gw_rules->rules[i]->src_ifindex == if_nametoindex("vcan0"));
		assert_true(gw_rules->rules[i]->dst_ifindex == if_nametoindex("vcan1"));
		assert_true(gw_rules->rules[i]->options == (SOCKETCAN_GW_RULE_ECHO | SOCKETCAN_GW_RULE_FILTER));
		assert_true(gw_rules->rules[i]->echo == 1);
		assert_true(gw_rules->rules[i]->filter.can_id == id);
		assert_true(gw_rules->rules[i]->filter.can_mask == 0xfff);
	}

	cangw_release_rules(gw_rules);
}
//------------------------------------------------------------------------------------------
static void delete_rule_test_20(void **state) {
	int ret = -1;
	socketcan_gw_rule_t gw_rule;
	socketcan_gw_rules_t *gw_rules = NULL;

	memset(&gw_rule, 0, sizeof(gw_rule));

	gw_rule.src_ifindex = if_nametoindex("vcan0");
	gw_rule.dst_ifindex = if_nametoindex("vcan1");

	gw_rule.options |= SOCKETCAN_GW_RULE_ECHO;
	gw_rule.echo = 1;

	gw_rule.options |= SOCKETCAN_GW_RULE_FILTER;
	gw_rule.filter.can_id = 0x3C0;
	gw_rule.filter.can_mask = 0xff0;

	ret = cangw_add_rule(&gw_rule);
	assert_true(ret == 0);

	ret = cangw_delete_rule(&gw_rule);
	assert_true(ret == 0);

	ret = cangw_get_rules(&gw_rules);
	assert_true(ret == 0);
	assert_non_null(gw_rules);
	assert_true(gw_rules->rule_num == 0);

	cangw_release_rules(gw_rules);
}
//------------------------------------------------------------------------------------------
static void delete_rule_test_21(void **state) {
	int ret = -1;
	socketcan_gw_rule_t gw_rule;
	socketcan_gw_rules_t *gw_rules = NULL;

	memset(&gw_rule, 0, sizeof(gw_rule));

	gw_rule.src_ifindex = if_nametoindex("vcan0");
	gw_rule.dst_ifindex = if_nametoindex("vxcan0");

	gw_rule.options |= SOCKETCAN_GW_RULE_ECHO;
	gw_rule.echo = 0;

	gw_rule.options |= SOCKETCAN_GW_RULE_FILTER;
	gw_rule.filter.can_id = 0x123;
	gw_rule.filter.can_mask = 0x000;

	ret = cangw_add_rule(&gw_rule);
	assert_true(ret == 0);

	ret = cangw_delete_rule(&gw_rule);
	assert_true(ret == 0);

	ret = cangw_get_rules(&gw_rules);
	assert_true(ret == 0);
	assert_non_null(gw_rules);
	assert_true(gw_rules->rule_num == 0);

	cangw_release_rules(gw_rules);
}
//------------------------------------------------------------------------------------------
static void delete_rule_test_22(void **state) {
	int ret = -1;
	socketcan_gw_rule_t gw_rule;
	socketcan_gw_rules_t *gw_rules = NULL;

	memset(&gw_rule, 0, sizeof(gw_rule));

	gw_rule.src_ifindex = if_nametoindex("vxcan0");
	gw_rule.dst_ifindex = if_nametoindex("vcan0");

	gw_rule.options |= SOCKETCAN_GW_RULE_ECHO;
	gw_rule.echo = 0;

	gw_rule.options |= SOCKETCAN_GW_RULE_FILTER;
	gw_rule.filter.can_id = 0x188;
	gw_rule.filter.can_mask = 0xfff;

	ret = cangw_add_rule(&gw_rule);
	assert_true(ret == 0);

	ret = cangw_delete_rule(&gw_rule);
	assert_true(ret == 0);

	ret = cangw_get_rules(&gw_rules);
	assert_true(ret == 0);
	assert_non_null(gw_rules);
	assert_true(gw_rules->rule_num == 0);

	cangw_release_rules(gw_rules);
}
//------------------------------------------------------------------------------------------
static struct option long_options[] = {
	{"add-rule", no_argument, 0, 10},
	{"delete-rule", no_argument, 0, 20},
	{"clean-rule", no_argument, 0, 20},
	{0, 0, 0, 0},
};

int main(int argc, char *argv[])
{
	int ret = -1;

	ret = getopt_long(argc, argv, "", long_options, NULL);
	switch (ret) {
		case 10:
		{
			{
				const struct CMUnitTest tests[] = {
					cmocka_unit_test(add_rule_test_10),
				};
				(void) cmocka_run_group_tests(tests, rule_setup, rule_teardown);	
			}
			{
				const struct CMUnitTest tests[] = {
					cmocka_unit_test(add_rule_test_11),
				};
				(void) cmocka_run_group_tests(tests, rule_setup, rule_teardown);	
			}
			{
				const struct CMUnitTest tests[] = {
					cmocka_unit_test(add_rule_test_12),
				};
				(void) cmocka_run_group_tests(tests, rule_setup, rule_teardown);	
			}
			{
				const struct CMUnitTest tests[] = {
					cmocka_unit_test(add_rule_test_13),
				};
				(void) cmocka_run_group_tests(tests, rule_setup, rule_teardown);	
			}
			{
				const struct CMUnitTest tests[] = {
					cmocka_unit_test(add_rule_test_14),
				};
				(void) cmocka_run_group_tests(tests, rule_setup, rule_teardown);	
			}
			break;
		}
		case 20:
		{
			{
				const struct CMUnitTest tests[] = {
					cmocka_unit_test(delete_rule_test_20),
				};
				(void) cmocka_run_group_tests(tests, rule_setup, rule_teardown);	
			}
			{
				const struct CMUnitTest tests[] = {
					cmocka_unit_test(delete_rule_test_21),
				};
				(void) cmocka_run_group_tests(tests, rule_setup, rule_teardown);	
			}
			{
				const struct CMUnitTest tests[] = {
					cmocka_unit_test(delete_rule_test_22),
				};
				(void) cmocka_run_group_tests(tests, rule_setup, rule_teardown);	
			}
			break;
		}
		case 30:
		{

			break;
		}
		default:
		{
			(void) fprintf(stderr, "This option is not support.\n");
			break;
		}
	}

	return 0;
}