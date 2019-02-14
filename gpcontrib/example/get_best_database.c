#include "postgres.h"
#include "query.h"

#include "fmgr.h"

Datum		get_database_popularity(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(get_database_popularity);

#ifndef UNIT_TESTING
PG_MODULE_MAGIC;
#endif

Datum
get_database_popularity(PG_FUNCTION_ARGS)
{
	/* TODO: This isn't parsing args as we expect. */
	char *database_name = PG_GETARG_CSTRING(0);

	int rank = get_rank(database_name);
	char *message;
	if (rank == 0)
	{
		message = "rank";
	}
	PG_RETURN_TEXT_P(message);
}

