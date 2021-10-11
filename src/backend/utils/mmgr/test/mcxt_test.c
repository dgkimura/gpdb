#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmockery.h"

#include "../mcxt.c"

static void
test__MemoryContextContainsGenericAllocation_DoesNotContainNullPointer(void **state)
{
	MemoryContext mcxt = AllocSetContextCreate(ErrorContext,
													 "TestContext",
													 ALLOCSET_DEFAULT_MINSIZE,
													 ALLOCSET_DEFAULT_INITSIZE,
													 ALLOCSET_DEFAULT_MAXSIZE);
	void *pointer = NULL;

	bool contains = MemoryContextContainsGenericAllocation(mcxt, pointer);
	assert_false(contains);
}

static void
test__MemoryContextContainsGenericAllocation_DoesNotContainNullSharedHeader(void **state)
{
	MemoryContext mcxt = AllocSetContextCreate(ErrorContext,
													 "TestContext",
													 ALLOCSET_DEFAULT_MINSIZE,
													 ALLOCSET_DEFAULT_INITSIZE,
													 ALLOCSET_DEFAULT_MAXSIZE);

	StandardChunkHeader *header = malloc(sizeof(StandardChunkHeader) + STANDARDCHUNKHEADERSIZE);
	header->sharedHeader = NULL;

	void *pointer = header + STANDARDCHUNKHEADERSIZE;

	bool contains = MemoryContextContainsGenericAllocation(mcxt, pointer);
	assert_false(contains);
}

int
main(int argc, char* argv[])
{
	cmockery_parse_arguments(argc, argv);

	const UnitTest tests[] = {
		unit_test(test__MemoryContextContainsGenericAllocation_DoesNotContainNullPointer),
		unit_test(test__MemoryContextContainsGenericAllocation_DoesNotContainNullSharedHeader),
	};

	return run_tests(tests);
}

