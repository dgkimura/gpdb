#include <stdlib.h>
#include <stdio.h>

#include "test-helpers.h"
#include "query-helpers.h"

void
resetGpdbFiveDataDirectories(void)
{
	printf("\nMaking a copy of gpdb5 data directories.\n");
	system("rsync -a --delete ./gpdb5-data-copy/ ./gpdb5-data");
}

void
resetGpdbSixDataDirectories(void)
{
	printf("\nMaking a copy of gpdb6 data directories.\n");
	system("rsync -a --delete ./gpdb6-data-copy/ ./gpdb6-data");
}

PGconn *
connectToFiveInSchema(char *schema)
{
	PGconn *conn = connectToFive();
	char buffer[1000];

	sprintf(buffer, "CREATE SCHEMA %s;", schema);
	executeQueryClearResult(conn, buffer);
	sprintf(buffer, "SET search_path TO %s", schema);
	executeQueryClearResult(conn, buffer);
	return conn;
}

PGconn *
connectToSixInSchema(char *schema)
{
	PGconn *conn = connectToSix();
	char buffer[1000];

	sprintf(buffer, "CREATE SCHEMA %s;", schema);
	executeQueryClearResult(conn, buffer);
	sprintf(buffer, "SET search_path TO %s", schema);
	executeQueryClearResult(conn, buffer);
	return conn;
}

PGconn *
connectToFive()
{
	return connectTo(50000);
}

PGconn *
connectToSix()
{
	return connectTo(60000);
}
