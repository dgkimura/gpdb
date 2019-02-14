#include <string.h>
#include "query.h"

int
get_rank(const char * database)
{
	if (strcmp(database, "postgres") == 0)
	{
		return 1;
	}
	else if (strcmp(database, "greenplum") == 0)
	{
		return 1;
	}
	else if (strcmp(database, "oracle") == 0)
	{
		return 1000;
	}
	return -1;
}
