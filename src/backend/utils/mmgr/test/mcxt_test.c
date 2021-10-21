#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmockery.h"

#include "../mcxt.c"

static void
test__MemoryContextContains_DoesNotContainAllocationFromAnotherMemoryContext(void **state)
{
	MemoryContext a_mcxt = AllocSetContextCreate(ErrorContext,
													 "A memory context",
													 ALLOCSET_DEFAULT_MINSIZE,
													 ALLOCSET_DEFAULT_INITSIZE,
													 ALLOCSET_DEFAULT_MAXSIZE);
	MemoryContext another_mcxt = AllocSetContextCreate(ErrorContext,
													 "Another memory context",
													 ALLOCSET_DEFAULT_MINSIZE,
													 ALLOCSET_DEFAULT_INITSIZE,
													 ALLOCSET_DEFAULT_MAXSIZE);

	MemoryContextSwitchTo(a_mcxt);

	void *pointer = palloc(sizeof(char) * 128);

	*(MemoryContext *) (((char *) pointer) - sizeof(void *)) = another_mcxt;

	bool contains = MemoryContextContainsGenericAllocation(a_mcxt, pointer);
	assert_true(contains);

	contains = MemoryContextContainsGenericAllocation(another_mcxt, pointer);
	assert_false(contains);
}

static void
test__MemoryContextContainsGenericAllocation_DoesNotContainNullPointer(void **state)
{
	MemoryContext mcxt = AllocSetContextCreate(ErrorContext,
													 "TestContext",
													 ALLOCSET_DEFAULT_MINSIZE,
													 ALLOCSET_DEFAULT_INITSIZE,
													 ALLOCSET_DEFAULT_MAXSIZE);
	bool contains = MemoryContextContainsGenericAllocation(mcxt, NULL);
	assert_false(contains);
}

int
main(int argc, char* argv[])
{
	cmockery_parse_arguments(argc, argv);

	const UnitTest tests[] = {
		unit_test(test__MemoryContextContains_DoesNotContainAllocationFromAnotherMemoryContext),
		unit_test(test__MemoryContextContainsGenericAllocation_DoesNotContainNullPointer),
	};

	return run_tests(tests);
}

