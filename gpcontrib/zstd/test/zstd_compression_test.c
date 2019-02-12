#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmockery.h"
#include "utils.h"

#include "postgres.h"
#include "utils/memutils.h"
#include "utils/elog.h"

#define UNIT_TESTING

#include "postgres.h"
#include "../zstd_compression.c"


void
test_constructed_without_compress_type(void **state)
{
	StorageAttributes sa = {};

	EXPECT_ELOG(ERROR);

	/*
	 * We need a try/catch because we expect zstd_constructor to throw ERROR.
	 */
	PG_TRY();
	{
		/*
		 * zstd_constructor is a SQL function, so we need to do some setup to call it
		 * DirectFunctionCall is used as a convenience, since it does some of this setup
		 */
		DirectFunctionCall3Coll(zstd_constructor, NULL, NULL, &sa, false);
		assert_false("zstd_constructor did not throw error");
	}
	PG_CATCH();
	{
	}
	PG_END_TRY();
}

void
test_zero_complevel_is_set_to_one(void **state)
{
	StorageAttributes sa = { .comptype = "zstd", .complevel = 0 };
	DirectFunctionCall3Coll(zstd_constructor, NULL, NULL, &sa, false);

	assert_int_equal(1, sa.complevel);
}

void
test_compress_with_dst_size_too_small(void **state)
{
	StorageAttributes sa = { .comptype = "zstd", .complevel = 0 };
	int32 dst_used;
	CompressionState *cs = DirectFunctionCall3Coll(zstd_constructor, NULL, NULL, PointerGetDatum(&sa), BoolGetDatum(false));

	DirectFunctionCall6(zstd_compress,
		CStringGetDatum("abcde"), -42,
		NULL, NULL,
		&dst_used, cs);

	/* dst_sz is too small to compress "abcde" */
	assert_int_equal(-42, dst_used);

	DirectFunctionCall1(zstd_destructor, PointerGetDatum(cs));
}

int
main(int argc, char *argv[])
{
	cmockery_parse_arguments(argc, argv);

	const		UnitTest tests[] = {
		unit_test(test_constructed_without_compress_type),
		unit_test(test_zero_complevel_is_set_to_one),
		unit_test(test_compress_with_dst_size_too_small)
	};

	MemoryContextInit();

	return run_tests(tests);
}
