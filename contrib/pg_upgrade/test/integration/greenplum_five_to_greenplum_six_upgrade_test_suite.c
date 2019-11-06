#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include "cmockery.h"

#include "greenplum_five_to_greenplum_six_upgrade_test_suite.h"

#include "scenarios/partitioned_ao_table.h"
#include "scenarios/partitioned_heap_table.h"
#include "scenarios/exchange_partitioned_heap_table.h"
#include "scenarios/partitioned_heap_table_with_a_dropped_column.h"
#include "scenarios/heap_table.h"
#include "scenarios/subpartitioned_heap_table.h"
#include "scenarios/ao_table.h"
#include "scenarios/aocs_table.h"
#include "scenarios/data_checksum_mismatch.h"
#include "scenarios/pl_function.h"
#include "scenarios/user_defined_types.h"
#include "scenarios/external_tables.h"

#include "utilities/gpdb5-cluster.h"
#include "utilities/gpdb6-cluster.h"

#include "utilities/test-helpers.h"
#include "utilities/row-assertions.h"
#include "utilities/upgrade-helpers.h"

static void
setup(void **state)
{
	resetGpdbFiveDataDirectories();
	resetGpdbSixDataDirectories();

	matcher = NULL;
	match_failed = NULL;
}

static void
teardown(void **state)
{
	stopGpdbFiveCluster();
	stopGpdbSixCluster();
}

int givens_index = 0;
int thens_index = 0;

// TODO: Make this a dynamic array
UnitTest givens[3];
UnitTest thens[3];

void
unit_test_given(UnitTestFunction f, char *n)
{
	givens[givens_index++] = (UnitTest){ n, f, UNIT_TEST_FUNCTION_TYPE_SETUP};
}
void
unit_test_then(UnitTestFunction f, char *n)
{
	thens[thens_index++] = (UnitTest){ n, f, UNIT_TEST_FUNCTION_TYPE_TEST};
}


int
main(int argc, char *argv[])
{
	cmockery_parse_arguments(argc, argv);

	resetGpdbFiveDataDirectories();
	resetGpdbSixDataDirectories();

	//const		UnitTest tests[] = {
		//unit_test_setup_teardown(test_a_readable_external_table_can_be_upgraded, setup, teardown),
		//unit_test_setup_teardown(test_a_partition_table_with_default_partition_after_split_can_be_upgraded, setup, teardown),
		//unit_test_setup_teardown(test_a_partition_table_with_newly_added_range_partition_can_be_upgraded, setup, teardown),
		//unit_test_setup_teardown(test_a_partition_table_with_newly_added_list_partition_can_be_upgraded, setup, teardown),
		//unit_test_setup_teardown(test_an_ao_table_with_data_can_be_upgraded, setup, teardown),
		//unit_test_setup_teardown(test_an_aocs_table_with_data_can_be_upgraded, setup, teardown),
		//unit_test_setup_teardown(test_a_heap_table_with_data_can_be_upgraded, setup, teardown),
		//unit_test_setup_teardown(test_a_subpartitioned_heap_table_with_data_can_be_upgraded, setup, teardown),
		//unit_test_setup_teardown(test_a_partitioned_heap_table_with_data_can_be_upgraded, setup, teardown),
		//unit_test_setup_teardown(test_a_partitioned_ao_table_with_data_can_be_upgraded, setup, teardown),
		//unit_test_setup_teardown(test_a_partitioned_ao_table_with_data_on_multiple_segfiles_can_be_upgraded, setup, teardown),
//		unit_test(test_a_partitioned_aoco_table_with_data_can_be_upgraded),
//		unit_test(test_a_partitioned_aoco_table_with_data_on_multiple_segfiles_can_be_upgraded),
		//unit_test_setup_teardown(test_a_partitioned_heap_table_with_a_dropped_column_can_be_upgraded, setup, teardown),
		//unit_test_setup_teardown(test_a_plpgsql_function_can_be_upgraded, setup, teardown),
		//unit_test_setup_teardown(test_a_plpython_function_can_be_upgraded, setup, teardown),
		//unit_test_setup_teardown(test_an_user_defined_type_extension_can_be_upgraded, setup, teardown),
	//};

	test_a_partitioned_aoco_table_with_data_on_multiple_segfiles_can_be_upgraded();
	test_a_partitioned_aoco_table_with_data_can_be_upgraded();
	test_a_partitioned_ao_table_with_data_on_multiple_segfiles_can_be_upgraded();
	//setup_test_a_partitioned_ao_table_with_data_can_be_upgraded(given, &given_index, then, &then_index);


		//unit_test_setup_teardown(test_an_exchange_partitioned_heap_table_cannot_be_upgraded, setup, teardown),
		//unit_test_setup_teardown(test_clusters_with_different_checksum_version_cannot_be_upgraded, setup, teardown),

	startGpdbFiveCluster();
	run_tests(givens);
	stopGpdbFiveCluster();

	performUpgrade();

	startGpdbSixCluster();
	return run_tests(thens);
}
