#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmockery.h"

#include "postgres.h"

#define UNIT_TESTING

#include "fmgr.h"
#include "../query.h"
#include "../get_best_database.c"

void
a_test_that_does_something(void **state)
{
	char *message;
	expect_value(get_rank, database, "greenplum");
	will_return(get_rank, 0);

	message = DatumGetCString(DirectFunctionCall1(get_database_popularity, "greenplum"));
	assert_string_equal(message, "uhh..");
}

// TODO: Add another testcase that uses auto-generated mocking from greenplum (e.g. elog)

int
main(int argc, char *argv[])
{
	cmockery_parse_arguments(argc, argv);

	const		UnitTest tests[] = {
		unit_test(a_test_that_does_something)
	};

	return run_tests(tests);
}
