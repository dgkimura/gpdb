#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void
startGpdbFiveCluster(void)
{
	system(""
		". $PWD/gpdb5/greenplum_path.sh; "
		"export PGPORT=50000; "
		"export MASTER_DATA_DIRECTORY=$PWD/gpdb5-data/qddir/demoDataDir-1; "
		"$PWD/gpdb5/bin/gpstart -a --skip_standby_check --no_standby"
	);
}

static void
stopGpdbFiveCluster(void)
{
	system(""
	". $PWD/gpdb5/greenplum_path.sh; \n"
	"export PGPORT=50000; \n"
	"export MASTER_DATA_DIRECTORY=$PWD/gpdb5-data/qddir/demoDataDir-1; \n"
	"$PWD/gpdb5/bin/gpstop -af"
	);
}



int
main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("number of arguments: %d", argc);
		printf("\nusage: ./scripts/gpdb5-cluster [start|stop]\n");
		exit(1);
	}

	char	   *const command = argv[1];

	if (strncmp(command, "start", 5) == 0)
		startGpdbFiveCluster();

	if (strncmp(command, "stop", 4) == 0)
		stopGpdbFiveCluster();
}
