#include <stdlib.h>
#include <stdio.h>

#include "test-helpers.h"
#include "query-helpers.h"
#include "upgrade-helpers.h"
#include "pqexpbuffer.h"

void
initializePgUpgradStatus(void)
{
	initPQExpBuffer(&pg_upgrade_output);
	pg_upgrade_exit_status = 0;
}

void
resetPgUpgradeStatus(void)
{
	termPQExpBuffer(&pg_upgrade_output);
}

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
connectToFive()
{
	return connectTo(50000);
}

PGconn *
connectToSix()
{
	return connectTo(60000);
}
