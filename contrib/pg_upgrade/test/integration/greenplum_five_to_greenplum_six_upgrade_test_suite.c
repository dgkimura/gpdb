#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include "cmockery.h"

#include "scenarios/partitioned_ao_table.h"
#include "scenarios/partitioned_heap_table.h"
#include "scenarios/heterogeneous_partitioned_heap_table.h"
#include "scenarios/exchange_partitioned_heap_table.h"
#include "scenarios/partitioned_heap_table_with_a_dropped_column.h"
#include "scenarios/heap_table.h"
#include "scenarios/subpartitioned_heap_table.h"
#include "scenarios/ao_table.h"
#include "scenarios/aocs_table.h"
#include "scenarios/data_checksum_mismatch.h"

#include "utilities/gpdb5-cluster.h"
#include "utilities/gpdb6-cluster.h"

#include "utilities/upgrade-helpers.h"
#include "utilities/test-helpers.h"
#include "utilities/row-assertions.h"

static void
setup(void **state)
{
	initializePgUpgradeStatus();
	resetGpdbFiveDataDirectories();
	resetGpdbSixDataDirectories();

	matcher = NULL;
	match_failed = NULL;
}

static void
teardown(void **state)
{
	resetPgUpgradeStatus();
	stopGpdbFiveCluster();
	stopGpdbSixCluster();
}

int
main(int argc, char *argv[])
{
	cmockery_parse_arguments(argc, argv);

	const		UnitTest tests[] = {
		unit_test_setup_teardown(test_clusters_with_different_checksum_version_cannot_be_upgraded, setup, teardown),
		unit_test_setup_teardown(test_an_ao_table_with_data_can_be_upgraded, setup, teardown),
		unit_test_setup_teardown(test_an_aocs_table_with_data_can_be_upgraded, setup, teardown),
		unit_test_setup_teardown(test_a_heap_table_with_data_can_be_upgraded, setup, teardown),
		unit_test_setup_teardown(test_a_subpartitioned_heap_table_with_data_can_be_upgraded, setup, teardown),
		unit_test_setup_teardown(test_a_partitioned_heap_table_with_data_can_be_upgraded, setup, teardown),
		unit_test_setup_teardown(test_a_partitioned_ao_table_with_data_can_be_upgraded, setup, teardown),
		unit_test_setup_teardown(test_a_partitioned_ao_table_with_data_on_multiple_segfiles_can_be_upgraded, setup, teardown),
		unit_test_setup_teardown(test_a_partitioned_aoco_table_with_data_can_be_upgraded, setup, teardown),
		unit_test_setup_teardown(test_a_partitioned_aoco_table_with_data_on_multiple_segfiles_can_be_upgraded, setup, teardown),
		unit_test_setup_teardown(test_an_exchange_partitioned_heap_table_cannot_be_upgraded, setup, teardown),
		unit_test_setup_teardown(test_a_partitioned_heap_table_with_a_dropped_column_can_be_upgraded, setup, teardown),
		unit_test_setup_teardown(test_a_partitioned_heap_table_with_differently_sized_dropped_columns_cannot_be_upgraded, setup, teardown),
		unit_test_setup_teardown(test_a_partitioned_heap_table_with_differently_aligned_fixed_dropped_columns_cannot_be_upgraded, setup, teardown),
		unit_test_setup_teardown(test_a_partitioned_heap_table_with_differently_aligned_varlen_dropped_columns_cannot_be_upgraded, setup, teardown),
	};

	return run_tests(tests);
}
