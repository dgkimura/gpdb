#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmockery.h"

int
get_rank(const char * database)
{
	check_expected(database);
	return (int)mock();
}
